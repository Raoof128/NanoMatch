#include "nanomatch/book/order_book.hpp"

#include <iomanip>
#include <iostream>
#include <string>

namespace nm {

std::vector<Trade> OrderBook::add_order(Order order) {
    std::vector<Trade> trades;
    last_reject_ = RejectReason::None;

    // Turn away orders the book cannot represent, before anything is matched
    if (order.id == INVALID_ID) {
        last_reject_ = RejectReason::InvalidOrderId;
        return trades;
    }
    if (order.remaining_quantity == 0) {
        last_reject_ = RejectReason::InvalidQuantity;
        return trades;
    }
    if (order.type == OrderType::Limit && order.price <= 0) {
        last_reject_ = RejectReason::InvalidPrice;
        return trades;
    }
    // A second order under a resting order's ID would take over its lookup entry and
    // leave the first order on the book where no cancel could reach it.
    if (order_lookup_.find(order.id) != order_lookup_.end()) {
        last_reject_ = RejectReason::DuplicateOrderId;
        return trades;
    }
    if (order.initial_quantity < order.remaining_quantity) {
        order.initial_quantity = order.remaining_quantity;
    }

    // Assign timestamp
    order.timestamp = std::chrono::steady_clock::now();

    // Handle market orders: set aggressive price
    if (order.type == OrderType::Market) {
        order.set_market_price();
    }

    // FOK check: can we fill entirely?
    if (order.type == OrderType::FillOrKill) {
        if (!can_fill_completely(order)) {
            last_reject_ = RejectReason::FillOrKillUnfilled;
            return trades;  // Reject — no partial fills allowed
        }
    }

    // Attempt to match against opposite side
    trades = match_order(order);

    // Handle remaining quantity based on order type
    if (!order.is_filled()) {
        switch (order.type) {
            case OrderType::Limit:
                // Place remainder on the book
                place_on_book(std::move(order));
                break;

            case OrderType::Market:
            case OrderType::ImmediateOrCancel:
                // Cancel any unfilled remainder
                // (implicitly — just don't place on book)
                break;

            case OrderType::FillOrKill:
                // Should never reach here (pre-checked above)
                break;
        }
    }

    return trades;
}

std::vector<Trade> OrderBook::match_order(Order& incoming) {
    std::vector<Trade> trades;

    // Determine which side to match against
    bool is_buy = (incoming.side == Side::Buy);

    while (!incoming.is_filled()) {
        // Get the opposite side
        auto best = is_buy ? asks_.best_price() : bids_.best_price();
        if (!best)
            break;

        Price best_opposing = *best;

        // Check if prices cross
        bool crosses =
            is_buy ? (incoming.price >= best_opposing) : (incoming.price <= best_opposing);

        if (!crosses)
            break;  // No more matchable prices

        // Match against the front order at this price level
        auto& level = is_buy ? asks_.best_level() : bids_.best_level();
        Order& resting = level.front();

        // Trade quantity = min of both remaining quantities
        Quantity trade_qty = std::min(incoming.remaining_quantity, resting.remaining_quantity);

        // Trade price = resting order's price (price improvement for aggressor)
        Price trade_price = resting.price;

        // Record the trade
        Trade trade;
        trade.price = trade_price;
        trade.quantity = trade_qty;
        trade.timestamp = std::chrono::steady_clock::now();

        if (incoming.side == Side::Buy) {
            trade.buy_order_id = incoming.id;
            trade.sell_order_id = resting.id;
        } else {
            trade.buy_order_id = resting.id;
            trade.sell_order_id = incoming.id;
        }

        trades.push_back(trade);

        // Save resting order info before it may be removed
        OrderID resting_id = resting.id;
        bool resting_will_fill = (trade_qty >= resting.remaining_quantity);

        // Update incoming order
        incoming.fill(trade_qty);

        // Fill from the level (may pop front order if fully filled)
        level.fill_front(trade_qty);

        // Remove from lookup if resting order was fully filled
        if (resting_will_fill) {
            order_lookup_.erase(resting_id);
        }

        // Remove empty price level
        if (level.empty()) {
            if (is_buy) {
                asks_.remove_level_if_empty(best_opposing);
            } else {
                bids_.remove_level_if_empty(best_opposing);
            }
        }
    }

    return trades;
}

bool OrderBook::can_fill_completely(const Order& order) const {
    // 64-bit: a level's total can exceed what one order's 32-bit quantity can hold
    TotalQuantity remaining = order.remaining_quantity;

    if (order.side == Side::Buy) {
        // Check ask side
        for (const auto& [price, level] : asks_) {
            if (order.price < price)
                break;  // Can't match beyond this
            TotalQuantity available = level.total_quantity();
            if (available >= remaining)
                return true;
            remaining -= available;
        }
    } else {
        // Check bid side
        for (const auto& [price, level] : bids_) {
            if (order.price > price)
                break;
            TotalQuantity available = level.total_quantity();
            if (available >= remaining)
                return true;
            remaining -= available;
        }
    }

    return remaining == 0;
}

bool OrderBook::cancel_order(OrderID id) {
    auto it = order_lookup_.find(id);
    if (it == order_lookup_.end())
        return false;

    const auto& loc = it->second;
    if (loc.side == Side::Buy) {
        bids_.remove_order(loc.price, loc.iterator);
    } else {
        asks_.remove_order(loc.price, loc.iterator);
    }

    order_lookup_.erase(it);
    return true;
}

std::vector<Trade> OrderBook::modify_order(OrderID id, Price new_price, Quantity new_qty) {
    last_reject_ = RejectReason::None;

    // Check the request before touching the order, so a bad modify cannot delete it
    if (new_qty == 0) {
        last_reject_ = RejectReason::InvalidQuantity;
        return {};
    }
    if (new_price <= 0) {
        last_reject_ = RejectReason::InvalidPrice;
        return {};
    }

    auto it = order_lookup_.find(id);
    if (it == order_lookup_.end()) {
        last_reject_ = RejectReason::UnknownOrder;
        return {};
    }

    // Save order details before cancellation invalidates the iterator
    const OrderLocation loc = it->second;
    Side side = loc.side;
    OrderType type = loc.iterator->type;

    // A smaller quantity at the same price gives nothing up, so the order keeps its
    // place in the queue.
    if (new_price == loc.price && new_qty <= loc.iterator->remaining_quantity) {
        if (side == Side::Buy) {
            bids_.reduce_order(loc.price, loc.iterator, new_qty);
        } else {
            asks_.reduce_order(loc.price, loc.iterator, new_qty);
        }
        return {};
    }

    // Anything else is a new order for queueing purposes: cancel and add again
    cancel_order(id);

    // Re-add with new price/quantity (keeps same ID and original type)
    Order new_order;
    new_order.id = id;
    new_order.side = side;
    new_order.type = type;
    new_order.price = new_price;
    new_order.initial_quantity = new_qty;
    new_order.remaining_quantity = new_qty;

    return add_order(std::move(new_order));
}

void OrderBook::place_on_book(Order order) {
    OrderID id = order.id;
    Price price = order.price;
    Side side = order.side;

    PriceLevel::OrderIterator iter;
    if (side == Side::Buy) {
        iter = bids_.add_order(std::move(order));
    } else {
        iter = asks_.add_order(std::move(order));
    }

    order_lookup_[id] = OrderLocation{price, side, iter};
}

namespace {

// Checks one side of the book and adds up its orders. Returns false on the first problem.
// The description is only built when something is wrong, so a passing check stays cheap.
template <typename SideT, typename LookupT>
bool check_side(const SideT& levels, Side side, const LookupT& lookup, size_t& orders_seen,
                std::string& problem) {
    for (const auto& [price, level] : levels) {
        const auto level_problem = [&](const char* what) {
            problem =
                std::string(side_to_string(side)) + " level " + std::to_string(price) + " " + what;
            return false;
        };
        if (level.empty())
            return level_problem("is empty");
        if (level.price() != price)
            return level_problem("is filed under a different price");

        TotalQuantity sum = 0;
        for (auto it = level.begin(); it != level.end(); ++it) {
            const Order& order = *it;
            const auto order_problem = [&](const char* what) {
                problem = std::string(side_to_string(side)) + " level " + std::to_string(price) +
                          ", order " + std::to_string(order.id) + " " + what;
                return false;
            };
            if (order.side != side || order.price != price)
                return order_problem("is on the wrong side or level");
            if (order.type != OrderType::Limit)
                return order_problem("is resting but is not a limit order");
            if (order.remaining_quantity == 0 || order.remaining_quantity > order.initial_quantity)
                return order_problem("has an impossible remaining quantity");

            auto found = lookup.find(order.id);
            if (found == lookup.end())
                return order_problem("is missing from the ID lookup");
            if (found->second.price != price || found->second.side != side ||
                &*found->second.iterator != &order)
                return order_problem("has a lookup entry that points somewhere else");

            sum += order.remaining_quantity;
            ++orders_seen;
        }
        if (sum != level.total_quantity())
            return level_problem("total does not equal the sum of its orders");
    }
    return true;
}

}  // namespace

bool OrderBook::check_invariants(std::string* why) const {
    std::string problem;
    size_t orders_seen = 0;

    bool ok = check_side(bids_, Side::Buy, order_lookup_, orders_seen, problem) &&
              check_side(asks_, Side::Sell, order_lookup_, orders_seen, problem);

    if (ok && orders_seen != order_lookup_.size()) {
        problem = "the ID lookup holds " + std::to_string(order_lookup_.size()) +
                  " entries but the queues hold " + std::to_string(orders_seen) + " orders";
        ok = false;
    }

    auto bid = best_bid();
    auto ask = best_ask();
    if (ok && bid && ask && *bid >= *ask) {
        problem = "the book is crossed: best bid " + std::to_string(*bid) + ", best ask " +
                  std::to_string(*ask);
        ok = false;
    }

    if (!ok && why != nullptr) {
        *why = problem;
    }
    return ok;
}

void OrderBook::print_book(size_t depth) const {
    std::cout << "\n===== " << symbol_ << " Order Book =====\n";

    // Print asks (reversed — show highest to lowest for display)
    std::vector<std::pair<Price, TotalQuantity>> ask_display;
    size_t count = 0;
    for (const auto& [price, level] : asks_) {
        if (count++ >= depth)
            break;
        ask_display.emplace_back(price, level.total_quantity());
    }

    std::cout << "  ASKS:\n";
    for (auto it = ask_display.rbegin(); it != ask_display.rend(); ++it) {
        std::cout << "    " << std::setw(10) << price_to_string(it->first) << "  x " << it->second
                  << "\n";
    }

    // Spread
    auto sp = spread();
    std::cout << "  --- spread: " << (sp ? price_to_string(*sp) : "N/A") << " ---\n";

    // Print bids (highest to lowest — natural order for greater<>)
    std::cout << "  BIDS:\n";
    count = 0;
    for (const auto& [price, level] : bids_) {
        if (count++ >= depth)
            break;
        std::cout << "    " << std::setw(10) << price_to_string(price) << "  x "
                  << level.total_quantity() << "\n";
    }

    std::cout << "  Total orders: " << total_orders() << "\n";
    std::cout << "================================\n\n";
}

}  // namespace nm
