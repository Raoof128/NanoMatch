#pragma once
#include "nanomatch/book/book_side.hpp"
#include "nanomatch/core/trade.hpp"
#include <unordered_map>
#include <vector>
#include <string>

namespace nm {

/// Central limit order book for a single symbol.
/// Manages bid/ask sides, order matching, and fast O(1) order lookup.
class OrderBook {
public:
    explicit OrderBook(std::string symbol) : symbol_(std::move(symbol)) {}

    // ── Core Operations ──

    /// Add a new order, matching against the opposite side. Returns generated trades.
    std::vector<Trade> add_order(Order order);

    /// Cancel an existing order by ID. Returns true if the order was found and removed.
    bool cancel_order(OrderID id);

    /// Modify an existing order (cancel + re-add at new price/qty). Returns any new trades.
    std::vector<Trade> modify_order(OrderID id, Price new_price, Quantity new_qty);

    // ── Query ──

    /// Returns the best (highest) bid price, or nullopt if no bids
    [[nodiscard]] std::optional<Price> best_bid() const { return bids_.best_price(); }
    /// Returns the best (lowest) ask price, or nullopt if no asks
    [[nodiscard]] std::optional<Price> best_ask() const { return asks_.best_price(); }

    /// Returns the bid-ask spread, or nullopt if either side is empty
    [[nodiscard]] std::optional<Price> spread() const {
        auto bid = best_bid();
        auto ask = best_ask();
        if (bid && ask) return *ask - *bid;
        return std::nullopt;
    }

    [[nodiscard]] const std::string& symbol() const { return symbol_; }
    [[nodiscard]] size_t bid_levels() const { return bids_.level_count(); }
    [[nodiscard]] size_t ask_levels() const { return asks_.level_count(); }
    [[nodiscard]] size_t total_orders() const { return order_lookup_.size(); }

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
    BidSide     bids_;
    AskSide     asks_;

    // Fast lookup: OrderID → (price, side, iterator into price level's list)
    struct OrderLocation {
        Price                     price;
        Side                      side;
        PriceLevel::OrderIterator iterator;
    };

    std::unordered_map<OrderID, OrderLocation> order_lookup_;
};

} // namespace nm
