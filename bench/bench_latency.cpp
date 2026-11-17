// Per-call latency of OrderBook on one thread, by kind of call.
//
// Each add_order and cancel_order call is timed on its own with steady_clock, and the
// samples are split into three groups: an add that rests without trading, an add that
// trades on arrival, and a cancel of a resting order. Two properties of the clock limit
// what the numbers can show, so both are measured and printed: the cost of reading the
// clock (included in every sample) and its tick (samples are multiples of it).

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include "bench_common.hpp"

using namespace nm;

namespace {

using Clock = std::chrono::steady_clock;

uint64_t nanoseconds(Clock::time_point from, Clock::time_point to) {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(to - from).count());
}

uint64_t percentile(const std::vector<uint64_t>& sorted, double p) {
    if (sorted.empty())
        return 0;
    auto idx = static_cast<size_t>(p / 100.0 * static_cast<double>(sorted.size()));
    idx = std::min(idx, sorted.size() - 1);
    return sorted[idx];
}

void print_row(const std::string& label, std::vector<uint64_t>& samples) {
    std::sort(samples.begin(), samples.end());
    const double mean =
        samples.empty()
            ? 0.0
            : static_cast<double>(std::accumulate(samples.begin(), samples.end(), uint64_t{0})) /
                  static_cast<double>(samples.size());
    std::cout << "  " << std::left << std::setw(24) << label << std::right << std::setw(9)
              << samples.size() << std::setw(9) << std::fixed << std::setprecision(1) << mean
              << std::setw(8) << percentile(samples, 50) << std::setw(8) << percentile(samples, 90)
              << std::setw(8) << percentile(samples, 99) << std::setw(9)
              << percentile(samples, 99.9) << std::setw(11)
              << (samples.empty() ? 0 : samples.back()) << "\n";
}

}  // namespace

int main() {
    constexpr uint32_t kOperations = 1'000'000;
    constexpr Price kBasePrice = 10000;
    constexpr Price kPriceRange = 200;
    constexpr Quantity kMaxQuantity = 500;

    const auto workload =
        bench::make_workload(kOperations, 42, kBasePrice, kPriceRange, kMaxQuantity);

    // The clock itself: read it back to back many times.
    std::vector<uint64_t> clock_deltas(200'000);
    for (auto& delta : clock_deltas) {
        const auto t1 = Clock::now();
        const auto t2 = Clock::now();
        delta = nanoseconds(t1, t2);
    }
    std::sort(clock_deltas.begin(), clock_deltas.end());
    const uint64_t clock_cost = percentile(clock_deltas, 50);
    const auto first_tick = std::upper_bound(clock_deltas.begin(), clock_deltas.end(), 0u);
    const uint64_t clock_tick = (first_tick == clock_deltas.end()) ? 0 : *first_tick;

    std::vector<uint64_t> add_rested;
    std::vector<uint64_t> add_matched;
    std::vector<uint64_t> cancel_hit;
    add_rested.reserve(workload.adds);
    add_matched.reserve(workload.adds);
    cancel_hit.reserve(workload.cancels);
    uint64_t cancel_misses = 0;

    OrderBook book("LATENCY");
    for (const auto& step : workload.steps) {
        if (step.cancel_target != INVALID_ID) {
            const auto t1 = Clock::now();
            const bool removed = book.cancel_order(step.cancel_target);
            const auto t2 = Clock::now();
            if (removed)
                cancel_hit.push_back(nanoseconds(t1, t2));
            else
                ++cancel_misses;
        } else {
            Order order = bench::make_limit_order(step);
            const auto t1 = Clock::now();
            const auto trades = book.add_order(std::move(order));
            const auto t2 = Clock::now();
            (trades.empty() ? add_rested : add_matched).push_back(nanoseconds(t1, t2));
        }
    }

    std::cout << std::string(88, '=') << "\n";
    std::cout << "  NanoMatch latency benchmark (one thread, each call timed separately)\n";
    std::cout << std::string(88, '=') << "\n";
    bench::print_environment(std::cout);
    std::cout << "  Clock:       steady_clock; reading it twice in a row takes " << clock_cost
              << " ns (median),\n               smallest step seen " << clock_tick << " ns\n";
    std::cout << "  Operations:  " << kOperations << " (cancels that missed: " << cancel_misses
              << ")\n\n";
    std::cout << "  " << std::left << std::setw(24) << "Call (times in ns)" << std::right
              << std::setw(9) << "count" << std::setw(9) << "mean" << std::setw(8) << "p50"
              << std::setw(8) << "p90" << std::setw(8) << "p99" << std::setw(9) << "p99.9"
              << std::setw(11) << "max" << "\n";
    print_row("add, rests", add_rested);
    print_row("add, trades on arrival", add_matched);
    print_row("cancel, resting order", cancel_hit);
    std::cout << std::string(88, '=') << "\n";

    // Samples for scripts/plot_latency.py
    std::ofstream csv("latency_histogram.csv");
    csv << "op,latency_ns\n";
    for (auto ns : add_rested)
        csv << "add_rested," << ns << "\n";
    for (auto ns : add_matched)
        csv << "add_matched," << ns << "\n";
    for (auto ns : cancel_hit)
        csv << "cancel," << ns << "\n";
    csv.close();
    std::cout << "\nSamples written to latency_histogram.csv\n";

    return 0;
}
