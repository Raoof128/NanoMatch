#pragma once
#include <vector>
#include <algorithm>
#include <numeric>
#include <cstdint>

namespace nm {

class LatencyStats {
public:
    void record(uint64_t latency_ns) {
        latencies_.push_back(latency_ns);
    }

    void reserve(size_t count) {
        latencies_.reserve(count);
    }

    [[nodiscard]] uint64_t percentile(double p) const {
        if (latencies_.empty()) return 0;
        auto sorted = latencies_;
        std::sort(sorted.begin(), sorted.end());
        size_t idx = static_cast<size_t>(p / 100.0 * sorted.size());
        idx = std::min(idx, sorted.size() - 1);
        return sorted[idx];
    }

    [[nodiscard]] double average() const {
        if (latencies_.empty()) return 0.0;
        return std::accumulate(latencies_.begin(), latencies_.end(), 0ULL) /
               static_cast<double>(latencies_.size());
    }

    [[nodiscard]] uint64_t max() const {
        if (latencies_.empty()) return 0;
        return *std::max_element(latencies_.begin(), latencies_.end());
    }

    [[nodiscard]] uint64_t min() const {
        if (latencies_.empty()) return 0;
        return *std::min_element(latencies_.begin(), latencies_.end());
    }

    [[nodiscard]] size_t count() const { return latencies_.size(); }

    [[nodiscard]] const std::vector<uint64_t>& data() const { return latencies_; }

private:
    std::vector<uint64_t> latencies_;
};

} // namespace nm
