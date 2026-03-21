#pragma once
#include "nanomatch/core/types.hpp"
#include "nanomatch/core/side.hpp"
#include <chrono>

namespace nm {

struct Trade {
    OrderID   buy_order_id;
    OrderID   sell_order_id;
    Price     price;           // Execution price (resting order's price)
    Quantity  quantity;         // Shares traded
    std::chrono::steady_clock::time_point timestamp;
};

} // namespace nm
