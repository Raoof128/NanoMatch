#pragma once
#include <chrono>
#include <cstdint>

namespace nm {

class ThroughputStats {
public:
    void start() {
        start_ = std::chrono::high_resolution_clock::now();
        ops_ = 0;
    }

    void record_op() {
        ++ops_;
    }

    void record_ops(uint64_t count) {
        ops_ += count;
    }

    void stop() {
        end_ = std::chrono::high_resolution_clock::now();
    }

    [[nodiscard]] double elapsed_seconds() const {
        auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(
            end_ - start_).count();
        return elapsed_us / 1'000'000.0;
    }

    [[nodiscard]] double ops_per_second() const {
        double elapsed = elapsed_seconds();
        return (elapsed > 0) ? ops_ / elapsed : 0.0;
    }

    [[nodiscard]] double avg_latency_ns() const {
        double elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(
            end_ - start_).count();
        return (ops_ > 0) ? (elapsed_us * 1000.0) / ops_ : 0.0;
    }

    [[nodiscard]] uint64_t total_ops() const { return ops_; }

private:
    std::chrono::high_resolution_clock::time_point start_;
    std::chrono::high_resolution_clock::time_point end_;
    uint64_t ops_ = 0;
};

} // namespace nm
