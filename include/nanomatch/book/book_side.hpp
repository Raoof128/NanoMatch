#pragma once
#include "nanomatch/core/price_level.hpp"
#include <map>
#include <functional>
#include <optional>

namespace nm {

// Template on comparator: std::greater for bids, std::less for asks
template <typename Compare = std::less<Price>>
class BookSide {
public:
    using LevelMap     = std::map<Price, PriceLevel, Compare>;
    using LevelIterator = typename LevelMap::iterator;

    [[nodiscard]] bool empty() const { return levels_.empty(); }

    // Best price level (front of sorted map)
    [[nodiscard]] std::optional<Price> best_price() const {
        if (levels_.empty()) return std::nullopt;
        return levels_.begin()->first;
    }

    [[nodiscard]] PriceLevel& best_level() {
        return levels_.begin()->second;
    }

    // Add order to appropriate price level
    PriceLevel::OrderIterator add_order(Order order) {
        Price p = order.price;
        // Create price level if it doesn't exist
        auto [it, _] = levels_.try_emplace(p, p);
        return it->second.add_order(std::move(order));
    }

    // Remove order from specific price level
    void remove_order(Price price, PriceLevel::OrderIterator order_it) {
        auto level_it = levels_.find(price);
        if (level_it == levels_.end()) return;

        level_it->second.remove_order(order_it);

        // Clean up empty price levels
        if (level_it->second.empty()) {
            levels_.erase(level_it);
        }
    }

    // Check if incoming price crosses best price on this side
    [[nodiscard]] bool crosses(Price incoming_price, Side incoming_side) const {
        if (levels_.empty()) return false;
        Price best = levels_.begin()->first;
        if (incoming_side == Side::Buy) {
            return incoming_price >= best;  // buy crosses asks
        } else {
            return incoming_price <= best;  // sell crosses bids
        }
    }

    // Remove empty price level by price
    void remove_level_if_empty(Price price) {
        auto level_it = levels_.find(price);
        if (level_it != levels_.end() && level_it->second.empty()) {
            levels_.erase(level_it);
        }
    }

    // Get number of price levels
    [[nodiscard]] size_t level_count() const { return levels_.size(); }

    // Iterate levels for display
    auto begin() { return levels_.begin(); }
    auto end()   { return levels_.end(); }
    auto begin() const { return levels_.begin(); }
    auto end()   const { return levels_.end(); }

private:
    LevelMap levels_;
};

// Type aliases
using BidSide = BookSide<std::greater<Price>>;  // Highest price first
using AskSide = BookSide<std::less<Price>>;     // Lowest price first

} // namespace nm
