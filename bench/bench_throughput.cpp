// Throughput of OrderBook on one thread, replaying a synthetic workload.
//
// What is timed: add_order and cancel_order calls only. The workload, including which
// resting order each cancel names, is prepared beforehand (see bench_common.hpp).
// What is not measured: order entry, parsing, validation, the MatchingEngine layer,
// networking, or more than one thread.

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <vector>

#include "bench_common.hpp"

using namespace nm;

namespace {

struct RunResult {
    double seconds = 0.0;
    uint64_t adds_rested = 0;   // added with no trade
    uint64_t adds_matched = 0;  // added and traded at least once
    uint64_t cancels_removed = 0;
    uint64_t cancels_missed = 0;
    uint64_t trades = 0;
    size_t book_orders = 0;
    size_t bid_levels = 0;
    size_t ask_levels = 0;
};

RunResult replay(const bench::Workload& workload) {
    RunResult result;
    OrderBook book("BENCH");

    const auto start = std::chrono::steady_clock::now();
    for (const auto& step : workload.steps) {
        if (step.cancel_target != INVALID_ID) {
            if (book.cancel_order(step.cancel_target))
                ++result.cancels_removed;
            else
                ++result.cancels_missed;
        } else {
            const auto trades = book.add_order(bench::make_limit_order(step));
            result.trades += trades.size();
            if (trades.empty())
                ++result.adds_rested;
            else
                ++result.adds_matched;
        }
    }
    const auto end = std::chrono::steady_clock::now();

    result.seconds = std::chrono::duration<double>(end - start).count();
    result.book_orders = book.total_orders();
    result.bid_levels = book.bid_levels();
    result.ask_levels = book.ask_levels();
    return result;
}

}  // namespace

int main() {
    constexpr uint32_t kOperations = 5'000'000;
    constexpr int kRuns = 5;
    constexpr Price kBasePrice = 10000;  // $100.00
    constexpr Price kPriceRange = 200;   // +/- $2.00
    constexpr Quantity kMaxQuantity = 1000;

    const auto workload =
        bench::make_workload(kOperations, 42, kBasePrice, kPriceRange, kMaxQuantity);

    std::vector<RunResult> runs;
    runs.reserve(kRuns);
    for (int i = 0; i < kRuns; ++i)
        runs.push_back(replay(workload));

    std::vector<double> throughput;
    for (const auto& run : runs)
        throughput.push_back(static_cast<double>(kOperations) / run.seconds);
    std::sort(throughput.begin(), throughput.end());

    const RunResult& first = runs.front();
    const auto line = [](const char* label, uint64_t value) {
        std::cout << "  " << std::left << std::setw(34) << label << std::right << std::setw(12)
                  << value << "\n";
    };

    std::cout << std::string(50, '=') << "\n";
    std::cout << "  NanoMatch throughput benchmark (one thread)\n";
    std::cout << std::string(50, '=') << "\n";
    bench::print_environment(std::cout);
    std::cout << "\n";
    line("Operations per run:", kOperations);
    line("  adds:", workload.adds);
    line("    rested without trading:", first.adds_rested);
    line("    traded on arrival:", first.adds_matched);
    line("  cancels:", workload.cancels);
    line("    removed a resting order:", first.cancels_removed);
    line("    missed (order not resting):", first.cancels_missed);
    line("Trades:", first.trades);
    line("Orders resting at the end:", first.book_orders);
    line("Bid levels at the end:", first.bid_levels);
    line("Ask levels at the end:", first.ask_levels);
    std::cout << "\n  Throughput over " << kRuns << " runs of the same workload (ops/s):\n";
    std::cout << std::fixed << std::setprecision(0);
    std::cout << "    slowest " << throughput.front() << "   median "
              << throughput[throughput.size() / 2] << "   fastest " << throughput.back() << "\n";
    std::cout << std::setprecision(1) << "  Mean time per operation, median run: "
              << 1e9 / throughput[throughput.size() / 2] << " ns\n";
    std::cout << std::string(50, '=') << "\n";

    return 0;
}
