#include "nanomatch/book/order_book.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

using namespace nm;

int main() {
    constexpr uint32_t NUM_OPERATIONS = 5'000'000;
    constexpr Price BASE_PRICE = 10000;  // $100.00
    constexpr Price PRICE_RANGE = 200;   // +/- $2.00

    OrderBook book("BENCH");

    std::mt19937 rng(42);  // Fixed seed for reproducibility
    std::uniform_int_distribution<int> side_dist(0, 1);
    std::uniform_int_distribution<Price> price_dist(BASE_PRICE - PRICE_RANGE,
                                                    BASE_PRICE + PRICE_RANGE);
    std::uniform_int_distribution<Quantity> qty_dist(1, 1000);
    std::uniform_int_distribution<int> action_dist(0, 9);

    // Pre-generate all orders (avoid RNG in timing loop)
    struct OpData {
        bool is_cancel;
        Side side;
        Price price;
        Quantity quantity;
    };

    std::vector<OpData> operations(NUM_OPERATIONS);
    for (auto& op : operations) {
        op.is_cancel = (action_dist(rng) < 2);  // 20% cancels
        op.side = side_dist(rng) ? Side::Buy : Side::Sell;
        op.price = price_dist(rng);
        op.quantity = qty_dist(rng);
    }

    // ── BENCHMARK ──
    OrderID id = 1;
    uint64_t total_trades = 0;

    auto start = std::chrono::high_resolution_clock::now();

    for (const auto& op : operations) {
        if (op.is_cancel && id > 1) {
            // Cancel a recent order
            book.cancel_order(id - 1);
        } else {
            Order order;
            order.id = id++;
            order.side = op.side;
            order.type = OrderType::Limit;
            order.price = op.price;
            order.initial_quantity = op.quantity;
            order.remaining_quantity = op.quantity;

            auto trades = book.add_order(std::move(order));
            total_trades += trades.size();
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    double elapsed_s = elapsed_us / 1'000'000.0;
    double throughput = NUM_OPERATIONS / elapsed_s;
    double avg_latency_ns = (elapsed_us * 1000.0) / NUM_OPERATIONS;

    std::cout << std::string(41, '=') << "\n";
    std::cout << "     NanoMatch Throughput Benchmark\n";
    std::cout << std::string(41, '=') << "\n";
    std::cout << "  Operations:  " << std::setw(12) << NUM_OPERATIONS << "\n";
    std::cout << "  Elapsed:     " << std::setw(12) << std::fixed << std::setprecision(2)
              << elapsed_s << " s\n";
    std::cout << "  Throughput:  " << std::setw(12) << static_cast<uint64_t>(throughput)
              << " ops/s\n";
    std::cout << "  Avg latency: " << std::setw(12) << std::setprecision(1) << avg_latency_ns
              << " ns\n";
    std::cout << "  Trades:      " << std::setw(12) << total_trades << "\n";
    std::cout << "  Book orders: " << std::setw(12) << book.total_orders() << "\n";
    std::cout << "  Bid levels:  " << std::setw(12) << book.bid_levels() << "\n";
    std::cout << "  Ask levels:  " << std::setw(12) << book.ask_levels() << "\n";
    std::cout << std::string(41, '=') << "\n";

    return 0;
}
