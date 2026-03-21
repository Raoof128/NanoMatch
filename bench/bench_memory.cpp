#include "nanomatch/memory/pool_allocator.hpp"
#include "nanomatch/core/order.hpp"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>

using namespace nm;

int main() {
    constexpr size_t POOL_SIZE = 1'000'000;
    constexpr size_t NUM_OPS = 500'000;

    std::cout << std::string(45, '=') << "\n";
    std::cout << "      NanoMatch Memory Pool Benchmark\n";
    std::cout << std::string(45, '=') << "\n";

    // Benchmark pool allocator
    {
        PoolAllocator<Order> pool(POOL_SIZE);

        auto start = std::chrono::high_resolution_clock::now();

        std::vector<Order*> allocated;
        allocated.reserve(NUM_OPS);

        // Allocate
        for (size_t i = 0; i < NUM_OPS; ++i) {
            Order* ptr = pool.allocate();
            if (ptr) allocated.push_back(ptr);
        }

        // Deallocate half
        for (size_t i = 0; i < allocated.size() / 2; ++i) {
            pool.deallocate(allocated[i]);
        }

        // Reallocate
        for (size_t i = 0; i < allocated.size() / 2; ++i) {
            pool.allocate();
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            end - start).count();

        double total_ops = NUM_OPS + NUM_OPS;  // alloc + dealloc/realloc
        double avg_ns = elapsed_ns / total_ops;

        std::cout << "  Pool allocator:\n";
        std::cout << "    Pool size:     " << std::setw(10) << POOL_SIZE << " objects\n";
        std::cout << "    Operations:    " << std::setw(10) << static_cast<uint64_t>(total_ops) << "\n";
        std::cout << "    Total time:    " << std::setw(10) << elapsed_ns / 1000 << " us\n";
        std::cout << "    Avg per op:    " << std::setw(10) << std::fixed
                  << std::setprecision(1) << avg_ns << " ns\n";
        std::cout << "    Order size:    " << std::setw(10) << sizeof(Order) << " bytes\n";
        std::cout << "    Pool memory:   " << std::setw(10)
                  << (POOL_SIZE * sizeof(Order)) / (1024 * 1024) << " MB\n";
    }

    // Benchmark new/delete for comparison
    {
        auto start = std::chrono::high_resolution_clock::now();

        std::vector<Order*> allocated;
        allocated.reserve(NUM_OPS);

        for (size_t i = 0; i < NUM_OPS; ++i) {
            allocated.push_back(new Order{});
        }

        for (size_t i = 0; i < allocated.size() / 2; ++i) {
            delete allocated[i];
        }

        for (size_t i = 0; i < allocated.size() / 2; ++i) {
            allocated[i] = new Order{};
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            end - start).count();

        double total_ops = NUM_OPS + NUM_OPS;
        double avg_ns = elapsed_ns / total_ops;

        std::cout << "\n  Standard new/delete:\n";
        std::cout << "    Operations:    " << std::setw(10) << static_cast<uint64_t>(total_ops) << "\n";
        std::cout << "    Total time:    " << std::setw(10) << elapsed_ns / 1000 << " us\n";
        std::cout << "    Avg per op:    " << std::setw(10) << std::fixed
                  << std::setprecision(1) << avg_ns << " ns\n";

        // Clean up remaining
        for (size_t i = allocated.size() / 2; i < allocated.size(); ++i) {
            delete allocated[i];
        }
    }

    std::cout << std::string(45, '=') << "\n";

    return 0;
}
