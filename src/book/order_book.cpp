#include "nanomatch/book/order_book.hpp"

#include <iomanip>
#include <iostream>

namespace nm {

std::vector<Trade> OrderBook::add_order(Order order) {
    std::vector<Trade> trades;

    // Assign timestamp
    order.timestamp = std::chrono::steady_clock::now();

    // Handle market orders: set aggressive price
    if (order.type == OrderType::Market) {
        order.set_market_price();
    }

    // FOK check: can we fill entirely?
    if (order.type == OrderType::FillOrKill) {
        if (!can_fill_completely(order)) {
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
    Quantity remaining = order.remaining_quantity;

    if (order.side == Side::Buy) {
        // Check ask side
        for (const auto& [price, level] : asks_) {
            if (order.price < price)
                break;  // Can't match beyond this
            Quantity available = level.total_quantity();
            if (available >= remaining)
                return true;
            remaining -= available;
        }
    } else {
        // Check bid side
        for (const auto& [price, level] : bids_) {
            if (order.price > price)
                break;
            Quantity available = level.total_quantity();
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
    auto it = order_lookup_.find(id);
    if (it == order_lookup_.end())
        return {};

    // Save order details before cancellation invalidates the iterator
    Side side = it->second.side;
    OrderType type = it->second.iterator->type;

    // Cancel existing
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

void OrderBook::print_book(size_t depth) const {
    std::cout << "\n===== " << symbol_ << " Order Book =====\n";

    // Print asks (reversed — show highest to lowest for display)
    std::vector<std::pair<Price, Quantity>> ask_display;
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
