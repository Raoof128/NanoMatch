#pragma once
#include "nanomatch/book/book_side.hpp"
#include "nanomatch/core/trade.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace nm {

/// Why the most recent add_order or modify_order call was turned away.
enum class RejectReason : uint8_t {
    None = 0,
    InvalidOrderId,      // the ID is INVALID_ID (0)
    InvalidQuantity,     // the quantity is zero
    InvalidPrice,        // a limit order, or a modify, priced at zero or below
    DuplicateOrderId,    // an order with this ID is already resting on the book
    UnknownOrder,        // modify: no resting order has this ID
    FillOrKillUnfilled,  // not enough resting quantity to fill the whole order
};

/// Central limit order book for a single symbol.
/// Manages bid/ask sides, order matching, and average O(1) order lookup.
///
/// Orders carry no owner or account, so the book cannot tell when both sides of a trade
/// belong to the same participant: there is no self-trade prevention.
class OrderBook {
public:
    explicit OrderBook(std::string symbol) : symbol_(std::move(symbol)) {}

    // ── Core Operations ──

    /// Add a new order, matching against the opposite side. Returns generated trades.
    ///
    /// The order is turned away, with no trades and no change to the book, when its ID is
    /// INVALID_ID or already resting, when its quantity is zero, when it is a limit order
    /// priced at zero or below, or when it is fill-or-kill and cannot be filled in full.
    /// last_reject() says which.
    std::vector<Trade> add_order(Order order);

    /// Cancel a resting order by ID. Returns true if the order was found and removed.
    /// An order that has been filled completely is no longer resting, so cancelling it
    /// returns false.
    bool cancel_order(OrderID id);

    /// Change a resting order's price or remaining quantity. Returns any new trades.
    ///
    /// `new_qty` is the new remaining quantity. Lowering it at an unchanged price keeps
    /// the order's place in the queue. Raising it, or changing the price, cancels the
    /// order and adds it again, so it joins the back of the queue and may match. A zero
    /// quantity, a price at or below zero, or an unknown ID leaves the book unchanged;
    /// last_reject() says which.
    std::vector<Trade> modify_order(OrderID id, Price new_price, Quantity new_qty);

    // ── Query ──

    /// Returns the best (highest) bid price, or nullopt if no bids
    [[nodiscard]] std::optional<Price> best_bid() const {
        return bids_.best_price();
    }
    /// Returns the best (lowest) ask price, or nullopt if no asks
    [[nodiscard]] std::optional<Price> best_ask() const {
        return asks_.best_price();
    }

    /// Returns the bid-ask spread, or nullopt if either side is empty
    [[nodiscard]] std::optional<Price> spread() const {
        auto bid = best_bid();
        auto ask = best_ask();
        if (bid && ask)
            return *ask - *bid;
        return std::nullopt;
    }

    [[nodiscard]] const std::string& symbol() const {
        return symbol_;
    }
    [[nodiscard]] size_t bid_levels() const {
        return bids_.level_count();
    }
    [[nodiscard]] size_t ask_levels() const {
        return asks_.level_count();
    }
    [[nodiscard]] size_t total_orders() const {
        return order_lookup_.size();
    }

    /// True if an order with this ID is resting on the book
    [[nodiscard]] bool contains(OrderID id) const {
        return order_lookup_.find(id) != order_lookup_.end();
    }

    /// The resting order with this ID, or nullptr. The pointer is valid until the next
    /// call that changes the book.
    [[nodiscard]] const Order* find_order(OrderID id) const {
        auto it = order_lookup_.find(id);
        return (it != order_lookup_.end()) ? &*it->second.iterator : nullptr;
    }

    /// Why the most recent add_order or modify_order call was turned away, or
    /// RejectReason::None if it was accepted
    [[nodiscard]] RejectReason last_reject() const {
        return last_reject_;
    }

    /// Walk the whole book and check its internal consistency: the book is not crossed,
    /// no level is empty, each level's total equals the sum of its orders, and the ID
    /// lookup and the queues describe the same set of orders. O(orders); meant for tests.
    /// On failure returns false and, if `why` is given, describes the first problem found.
    [[nodiscard]] bool check_invariants(std::string* why = nullptr) const;

    /// Print the top N levels of the book to stdout
    void print_book(size_t depth = 5) const;

private:
    // ── Matching Logic ──

    // Match incoming order against opposite side
    std::vector<Trade> match_order(Order& order);

    // Check if order can be fully filled (for FOK)
    bool can_fill_completely(const Order& order) const;

    // Place remaining quantity on the book
    void place_on_book(Order order);

    // ── Data ──

    std::string symbol_;
    BidSide bids_;
    AskSide asks_;

    // Fast lookup: OrderID → (price, side, iterator into price level's list)
    struct OrderLocation {
        Price price;
        Side side;
        PriceLevel::OrderIterator iterator;
    };

    std::unordered_map<OrderID, OrderLocation> order_lookup_;
    RejectReason last_reject_ = RejectReason::None;
};

}  // namespace nm
