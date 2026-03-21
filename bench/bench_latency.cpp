#include "nanomatch/book/order_book.hpp"
#include <chrono>
#include <iostream>
#include <random>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <numeric>
#include <fstream>

using namespace nm;

int main() {
    constexpr uint32_t NUM_OPS    = 1'000'000;
    constexpr Price    BASE_PRICE = 10000;
    constexpr Price    RANGE      = 200;

    OrderBook book("LATENCY");

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> side_dist(0, 1);
    std::uniform_int_distribution<Price> price_dist(BASE_PRICE - RANGE,
                                                     BASE_PRICE + RANGE);
    std::uniform_int_distribution<Quantity> qty_dist(1, 500);

    // Collect per-operation latencies
    std::vector<uint64_t> latencies_ns(NUM_OPS);

    OrderID id = 1;

    for (uint32_t i = 0; i < NUM_OPS; ++i) {
        Order order;
        order.id                = id++;
        order.side              = side_dist(rng) ? Side::Buy : Side::Sell;
        order.type              = OrderType::Limit;
        order.price             = price_dist(rng);
        order.initial_quantity  = qty_dist(rng);
        order.remaining_quantity = order.initial_quantity;

        auto t1 = std::chrono::high_resolution_clock::now();
        book.add_order(std::move(order));
        auto t2 = std::chrono::high_resolution_clock::now();

        latencies_ns[i] = static_cast<uint64_t>(std::chrono::duration_cast<
            std::chrono::nanoseconds>(t2 - t1).count());
    }

    // Sort for percentile calculation
    std::sort(latencies_ns.begin(), latencies_ns.end());

    auto percentile = [&](double p) -> uint64_t {
        size_t idx = static_cast<size_t>(p / 100.0 * latencies_ns.size());
        idx = std::min(idx, latencies_ns.size() - 1);
        return latencies_ns[idx];
    };

    double avg = std::accumulate(latencies_ns.begin(), latencies_ns.end(),
                                  0ULL) / static_cast<double>(NUM_OPS);

    std::cout << std::string(41, '=') << "\n";
    std::cout << "      NanoMatch Latency Benchmark\n";
    std::cout << std::string(41, '=') << "\n";
    std::cout << "  Operations:  " << std::setw(14) << NUM_OPS << "\n";
    std::cout << "  Avg:         " << std::setw(10) << std::fixed
              << std::setprecision(1) << avg << " ns\n";
    std::cout << "  p50:         " << std::setw(10) << percentile(50) << " ns\n";
    std::cout << "  p90:         " << std::setw(10) << percentile(90) << " ns\n";
    std::cout << "  p95:         " << std::setw(10) << percentile(95) << " ns\n";
    std::cout << "  p99:         " << std::setw(10) << percentile(99) << " ns\n";
    std::cout << "  p99.9:       " << std::setw(10) << percentile(99.9) << " ns\n";
    std::cout << "  Max:         " << std::setw(10) << latencies_ns.back() << " ns\n";
    std::cout << std::string(41, '=') << "\n";

    // Output CSV for histogram plotting
    std::ofstream csv("latency_histogram.csv");
    csv << "latency_ns\n";
    for (auto ns : latencies_ns) csv << ns << "\n";
    csv.close();
    std::cout << "\nHistogram data written to latency_histogram.csv\n";

    return 0;
}
