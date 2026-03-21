#pragma once
#include "nanomatch/core/types.hpp"
#include <cstdint>
#include <string_view>

namespace nm {

enum class Side : uint8_t {
    Buy  = 0,
    Sell = 1
};

constexpr std::string_view side_to_string(Side s) {
    return s == Side::Buy ? "BUY" : "SELL";
}

// Buy side: best = highest price. Sell side: best = lowest price.
// Returns true if `a` is a better price than `b` for the given side.
constexpr bool is_better_price(Side side, Price a, Price b) {
    return side == Side::Buy ? (a > b) : (a < b);
}

} // namespace nm
