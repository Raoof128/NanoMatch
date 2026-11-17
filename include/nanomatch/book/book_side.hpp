#pragma once
#include "nanomatch/core/price_level.hpp"

#include <functional>
#include <map>
#include <optional>

namespace nm {

/// One side of the order book (bid or ask), ordered by price.
/// Templated on comparator: std::greater for bids (highest first),
/// std::less for asks (lowest first).
template <typename Compare = std::less<Price>>
class BookSide {
public:
    using LevelMap = std::map<Price, PriceLevel, Compare>;
    using LevelIterator = typename LevelMap::iterator;

    [[nodiscard]] bool empty() const {
        return levels_.empty();
    }

    /// Returns the best price on this side, or nullopt if empty
    [[nodiscard]] std::optional<Price> best_price() const {
        if (levels_.empty())
            return std::nullopt;
        return levels_.begin()->first;
    }

    /// Returns a reference to the best (front) price level
    [[nodiscard]] PriceLevel& best_level() {
        return levels_.begin()->second;
    }

    /// Add an order to the appropriate price level (creates level if needed)
    PriceLevel::OrderIterator add_order(Order order) {
        Price p = order.price;
        // Create price level if it doesn't exist
        auto [it, _] = levels_.try_emplace(p, p);
        return it->second.add_order(std::move(order));
    }

    /// Remove an order from a specific price level; cleans up empty levels
    void remove_order(Price price, PriceLevel::OrderIterator order_it) {
        auto level_it = levels_.find(price);
        if (level_it == levels_.end())
            return;

        level_it->second.remove_order(order_it);

        // Clean up empty price levels
        if (level_it->second.empty()) {
            levels_.erase(level_it);
        }
    }

    /// Lower a resting order's remaining quantity without moving it in its queue.
    bool reduce_order(Price price, PriceLevel::OrderIterator order_it, Quantity new_remaining) {
        auto level_it = levels_.find(price);
        if (level_it == levels_.end())
            return false;
        return level_it->second.reduce_order(order_it, new_remaining);
    }

    /// Check if an incoming price crosses the best price on this side
    [[nodiscard]] bool crosses(Price incoming_price, Side incoming_side) const {
        if (levels_.empty())
            return false;
        Price best = levels_.begin()->first;
        if (incoming_side == Side::Buy) {
            return incoming_price >= best;  // buy crosses asks
        } else {
            return incoming_price <= best;  // sell crosses bids
        }
    }

    /// Remove a price level if it has no orders remaining
    void remove_level_if_empty(Price price) {
        auto level_it = levels_.find(price);
        if (level_it != levels_.end() && level_it->second.empty()) {
            levels_.erase(level_it);
        }
    }

    [[nodiscard]] size_t level_count() const {
        return levels_.size();
    }

    // Iterate levels for display
    auto begin() {
        return levels_.begin();
    }
    auto end() {
        return levels_.end();
    }
    auto begin() const {
        return levels_.begin();
    }
    auto end() const {
        return levels_.end();
    }

private:
    LevelMap levels_;
};

// Type aliases
using BidSide = BookSide<std::greater<Price>>;  // Highest price first
using AskSide = BookSide<std::less<Price>>;     // Lowest price first

}  // namespace nm
