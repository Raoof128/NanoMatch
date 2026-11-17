#pragma once
#include <algorithm>
#include <cstdint>
#include <numeric>
#include <vector>

namespace nm {

class LatencyStats {
public:
    void record(uint64_t latency_ns) {
        latencies_.push_back(latency_ns);
        computed_ = false;
    }

    void reserve(size_t count) {
        latencies_.reserve(count);
    }

    /// Sort the latencies once so that subsequent percentile() calls are O(1).
    /// Called automatically by percentile() if needed.
    void compute() {
        if (computed_)
            return;
        sorted_ = latencies_;
        std::sort(sorted_.begin(), sorted_.end());
        computed_ = true;
    }

    [[nodiscard]] uint64_t percentile(double p) const {
        if (latencies_.empty())
            return 0;
        // Use pre-sorted data if available, otherwise sort on demand
        if (!computed_) {
            const_cast<LatencyStats*>(this)->compute();
        }
        size_t idx = static_cast<size_t>(p / 100.0 * sorted_.size());
        idx = std::min(idx, sorted_.size() - 1);
        return sorted_[idx];
    }

    [[nodiscard]] double average() const {
        if (latencies_.empty())
            return 0.0;
        return std::accumulate(latencies_.begin(), latencies_.end(), 0ULL) /
               static_cast<double>(latencies_.size());
    }

    [[nodiscard]] uint64_t max() const {
        if (latencies_.empty())
            return 0;
        return *std::max_element(latencies_.begin(), latencies_.end());
    }

    [[nodiscard]] uint64_t min() const {
        if (latencies_.empty())
            return 0;
        return *std::min_element(latencies_.begin(), latencies_.end());
    }

    [[nodiscard]] size_t count() const {
        return latencies_.size();
    }

    [[nodiscard]] const std::vector<uint64_t>& data() const {
        return latencies_;
    }

private:
    std::vector<uint64_t> latencies_;
    std::vector<uint64_t> sorted_;  // Cached sorted copy
    bool computed_ = false;
};

}  // namespace nm
