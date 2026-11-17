#pragma once
#include <cstdint>
#include <limits>
#include <string>

namespace nm {

// Price stored as integer (cents) to avoid floating-point issues
// $101.50 = 10150
using Price = int32_t;
using Quantity = uint32_t;
using OrderID = uint64_t;

// Sum of the quantities resting at one price level. A single order holds at most
// 2^32 - 1 shares, so a level's total needs more than 32 bits.
using TotalQuantity = uint64_t;

// Sentinel values
constexpr Price INVALID_PRICE = 0;
constexpr Price MAX_PRICE = std::numeric_limits<Price>::max();
constexpr Price MIN_PRICE = std::numeric_limits<Price>::min();
constexpr OrderID INVALID_ID = 0;

// Convert price integer to display string: 10150 → "101.50"
inline std::string price_to_string(Price p) {
    // Widen before negating: -MIN_PRICE does not fit in 32 bits.
    int64_t cents = p;
    const bool negative = cents < 0;
    if (negative)
        cents = -cents;
    return (negative ? "-" : "") + std::to_string(cents / 100) + "." +
           (cents % 100 < 10 ? "0" : "") + std::to_string(cents % 100);
}

}  // namespace nm
