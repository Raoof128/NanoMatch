#pragma once
#include <cstdint>

namespace nm {

enum class OrderType : uint8_t {
    Limit = 0,              // Rests on book at specified price
    Market = 1,             // Execute immediately at best available
    ImmediateOrCancel = 2,  // Fill what you can, cancel remainder
    FillOrKill = 3,         // Fill entirely or cancel entirely
};

enum class TimeInForce : uint8_t {
    GoodTillCancel = 0,
    Day = 1,
};

}  // namespace nm
