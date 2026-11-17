// PoolAllocator against new/delete, on its own.
//
// The order book does not use PoolAllocator, so this says nothing about matching speed.
// The two sides are also not doing the same work: the pool builds all of its objects
// before the timed section starts (that cost is printed separately), while new/delete
// pays for construction inside it.

#include "nanomatch/core/order.hpp"
#include "nanomatch/memory/pool_allocator.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <vector>

#include "bench_common.hpp"

using namespace nm;

namespace {

using Clock = std::chrono::steady_clock;

double microseconds(Clock::time_point from, Clock::time_point to) {
    return std::chrono::duration<double, std::micro>(to - from).count();
}

}  // namespace

int main() {
    constexpr size_t kPoolSize = 1'000'000;
    constexpr size_t kObjects = 500'000;
    const double total_ops = static_cast<double>(kObjects) * 2.0;  // allocate, free half, refill

    std::cout << std::string(52, '=') << "\n";
    std::cout << "  NanoMatch pool allocator benchmark (standalone)\n";
    std::cout << std::string(52, '=') << "\n";
    bench::print_environment(std::cout);
    std::cout << std::fixed << std::setprecision(1);

    {
        const auto build_start = Clock::now();
        PoolAllocator<Order> pool(kPoolSize);
        const auto start = Clock::now();

        std::vector<Order*> allocated;
        allocated.reserve(kObjects);

        for (size_t i = 0; i < kObjects; ++i) {
            Order* ptr = pool.allocate();
            if (ptr)
                allocated.push_back(ptr);
        }
        for (size_t i = 0; i < allocated.size() / 2; ++i) {
            pool.deallocate(allocated[i]);
        }
        for (size_t i = 0; i < allocated.size() / 2; ++i) {
            allocated[i] = pool.allocate();
        }

        const auto end = Clock::now();
        const double elapsed_us = microseconds(start, end);

        std::cout << "\n  Pool allocator:\n";
        std::cout << "    Building the pool: " << std::setw(10) << microseconds(build_start, start)
                  << " us (" << kPoolSize << " objects, "
                  << (kPoolSize * sizeof(Order)) / (1024 * 1024)
                  << " MB, not in the figure below)\n";
        std::cout << "    Operations:        " << std::setw(10) << static_cast<uint64_t>(total_ops)
                  << "\n";
        std::cout << "    Total time:        " << std::setw(10) << elapsed_us << " us\n";
        std::cout << "    Mean per op:       " << std::setw(10) << elapsed_us * 1000.0 / total_ops
                  << " ns\n";
    }

    {
        const auto start = Clock::now();

        std::vector<Order*> allocated;
        allocated.reserve(kObjects);

        for (size_t i = 0; i < kObjects; ++i) {
            allocated.push_back(new Order{});
        }
        for (size_t i = 0; i < allocated.size() / 2; ++i) {
            delete allocated[i];
        }
        for (size_t i = 0; i < allocated.size() / 2; ++i) {
            allocated[i] = new Order{};
        }

        const auto end = Clock::now();
        const double elapsed_us = microseconds(start, end);

        std::cout << "\n  new/delete:\n";
        std::cout << "    Operations:        " << std::setw(10) << static_cast<uint64_t>(total_ops)
                  << "\n";
        std::cout << "    Total time:        " << std::setw(10) << elapsed_us << " us\n";
        std::cout << "    Mean per op:       " << std::setw(10) << elapsed_us * 1000.0 / total_ops
                  << " ns\n";

        for (Order* ptr : allocated) {
            delete ptr;
        }
    }

    std::cout << "\n  Order size: " << sizeof(Order) << " bytes\n";
    std::cout << std::string(52, '=') << "\n";

    return 0;
}
