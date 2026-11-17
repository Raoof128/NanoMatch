#pragma once
// Shared by the benchmarks: what was built, and the synthetic workload they replay.

#include "nanomatch/book/order_book.hpp"

#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <vector>

#ifndef NANOMATCH_BUILD_TYPE
#define NANOMATCH_BUILD_TYPE ""
#endif

namespace nm::bench {

inline void print_environment(std::ostream& out) {
    const std::string build_type = NANOMATCH_BUILD_TYPE;
#if defined(__clang__)
    out << "  Compiler:    clang " << __clang_version__ << "\n";
#elif defined(__GNUC__)
    out << "  Compiler:    gcc " << __VERSION__ << "\n";
#else
    out << "  Compiler:    unknown\n";
#endif
    out << "  Build type:  " << (build_type.empty() ? "(none set)" : build_type) << "\n";
#if defined(__OPTIMIZE__)
    out << "  Optimised:   yes\n";
#else
    out << "  Optimised:   NO. These figures are not meaningful; configure with "
           "-DCMAKE_BUILD_TYPE=Release\n";
#endif
}

/// One step of the workload. `cancel_target` is INVALID_ID for an add.
struct Step {
    OrderID id = INVALID_ID;  // the new order's ID (adds only)
    OrderID cancel_target = INVALID_ID;
    Price price = 0;
    Quantity quantity = 0;
    Side side = Side::Buy;
};

struct Workload {
    std::vector<Step> steps;
    uint64_t adds = 0;
    uint64_t cancels = 0;
};

inline Order make_limit_order(const Step& step) {
    Order order{};
    order.id = step.id;
    order.side = step.side;
    order.type = OrderType::Limit;
    order.price = step.price;
    order.initial_quantity = step.quantity;
    order.remaining_quantity = step.quantity;
    return order;
}

/// Builds the workload: limit orders at uniformly random prices within `price_range` of
/// `base_price`, with about one step in five cancelling an order.
///
/// Every cancel names an order that is resting at that point in the sequence, chosen
/// uniformly at random. To know which orders are resting, this function runs the whole
/// sequence through a scratch book once and follows the fills. That happens here, before
/// any timing; the timed replay only makes the calls. When no order is resting, the step
/// becomes an add.
inline Workload make_workload(uint32_t steps, uint32_t seed, Price base_price, Price price_range,
                              Quantity max_quantity) {
    constexpr uint32_t kAbsent = std::numeric_limits<uint32_t>::max();

    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> side_dist(0, 1);
    std::uniform_int_distribution<Price> price_dist(base_price - price_range,
                                                    base_price + price_range);
    std::uniform_int_distribution<Quantity> qty_dist(1, max_quantity);
    std::uniform_int_distribution<int> action_dist(0, 9);

    Workload workload;
    workload.steps.resize(steps);

    OrderBook scratch("PLAN");
    std::vector<OrderID> resting;                           // IDs resting right now
    std::vector<uint32_t> position(steps + 2ull, kAbsent);  // ID -> index in `resting`
    std::vector<Quantity> remaining(steps + 2ull, 0);       // ID -> unfilled quantity

    const auto forget = [&](OrderID id) {
        const uint32_t at = position[id];
        const OrderID moved = resting.back();
        resting[at] = moved;
        position[moved] = at;
        resting.pop_back();
        position[id] = kAbsent;
    };

    OrderID next_id = 1;
    for (auto& step : workload.steps) {
        const bool wants_cancel = (action_dist(rng) < 2);  // 20% of steps
        if (wants_cancel && !resting.empty()) {
            std::uniform_int_distribution<size_t> pick(0, resting.size() - 1);
            step.cancel_target = resting[pick(rng)];
            forget(step.cancel_target);
            scratch.cancel_order(step.cancel_target);
            ++workload.cancels;
            continue;
        }

        step.id = next_id++;
        step.side = side_dist(rng) ? Side::Buy : Side::Sell;
        step.price = price_dist(rng);
        step.quantity = qty_dist(rng);
        ++workload.adds;

        Quantity left = step.quantity;
        for (const auto& trade : scratch.add_order(make_limit_order(step))) {
            const OrderID other =
                (step.side == Side::Buy) ? trade.sell_order_id : trade.buy_order_id;
            left -= trade.quantity;
            remaining[other] -= trade.quantity;
            if (remaining[other] == 0)
                forget(other);
        }
        if (left > 0) {
            remaining[step.id] = left;
            position[step.id] = static_cast<uint32_t>(resting.size());
            resting.push_back(step.id);
        }
    }
    return workload;
}

}  // namespace nm::bench
