#pragma once
#include "nanomatch/core/types.hpp"
#include "nanomatch/core/side.hpp"
#include "nanomatch/core/order_type.hpp"
#include <chrono>

namespace nm {

/// Represents a single order in the matching engine, including its fill state.
struct Order {
    OrderID   id;
    Side      side;
    OrderType type;
    Price     price;            // 0 for market orders
    Quantity  initial_quantity;
    Quantity  remaining_quantity;
    std::chrono::steady_clock::time_point timestamp;

    /// Returns true if the order has been completely filled
    [[nodiscard]] bool is_filled() const {
        return remaining_quantity == 0;
    }

    /// Returns the quantity that has already been filled
    [[nodiscard]] Quantity filled_quantity() const {
        return initial_quantity - remaining_quantity;
    }

    /// Reduce remaining quantity by the given amount (clamped to zero)
    void fill(Quantity qty) {
        remaining_quantity -= std::min(qty, remaining_quantity);
    }

    /// For market orders, set aggressive price to guarantee crossing
    void set_market_price() {
        price = (side == Side::Buy) ? MAX_PRICE : MIN_PRICE;
    }
};

/// Incoming order request from external input (CSV, network, etc.)
struct OrderRequest {
    enum class Action : uint8_t { New, Cancel, Modify };

    Action    action;
    OrderID   id;
    Side      side;
    OrderType type;
    Price     price;
    Quantity  quantity;
};

} // namespace nm
