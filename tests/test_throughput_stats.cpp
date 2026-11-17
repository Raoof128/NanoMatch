#include "nanomatch/stats/throughput_stats.hpp"

#include <chrono>
#include <thread>

#include <gtest/gtest.h>

using namespace nm;

TEST(ThroughputStatsTest, StartStopRecordsTiming) {
    ThroughputStats stats;
    stats.start();
    // Busy-wait a small amount to ensure measurable elapsed time
    auto deadline = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(10);
    while (std::chrono::high_resolution_clock::now() < deadline) {}
    stats.stop();

    EXPECT_GT(stats.elapsed_seconds(), 0.0);
}

TEST(ThroughputStatsTest, RecordOpsCount) {
    ThroughputStats stats;
    stats.start();
    stats.record_op();
    stats.record_op();
    stats.record_op();
    stats.stop();

    EXPECT_EQ(stats.total_ops(), 3u);
}

TEST(ThroughputStatsTest, RecordOpsInBulk) {
    ThroughputStats stats;
    stats.start();
    stats.record_ops(100);
    stats.record_ops(50);
    stats.stop();

    EXPECT_EQ(stats.total_ops(), 150u);
}

TEST(ThroughputStatsTest, OpsPerSecondCalculation) {
    ThroughputStats stats;
    stats.start();

    for (int i = 0; i < 1000; ++i) {
        stats.record_op();
    }

    // Wait a small measurable time
    auto deadline = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(10);
    while (std::chrono::high_resolution_clock::now() < deadline) {}
    stats.stop();

    double ops_sec = stats.ops_per_second();
    EXPECT_GT(ops_sec, 0.0);
    // 1000 ops in ~10ms should be roughly 100,000 ops/sec
    // We allow a wide range since CI timing can vary
    EXPECT_GT(ops_sec, 1000.0);
}

TEST(ThroughputStatsTest, AvgLatencyCalculation) {
    ThroughputStats stats;
    stats.start();

    stats.record_ops(100);

    auto deadline = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(10);
    while (std::chrono::high_resolution_clock::now() < deadline) {}
    stats.stop();

    double avg_ns = stats.avg_latency_ns();
    EXPECT_GT(avg_ns, 0.0);
}

TEST(ThroughputStatsTest, ZeroOpsAvgLatencyIsZero) {
    ThroughputStats stats;
    stats.start();
    stats.stop();

    EXPECT_DOUBLE_EQ(stats.avg_latency_ns(), 0.0);
}

TEST(ThroughputStatsTest, ZeroOpsOpsPerSecondIsZero) {
    ThroughputStats stats;
    // start and immediately stop with no elapsed time difference
    stats.start();
    stats.stop();

    // With zero ops and near-zero time, ops_per_second should handle gracefully
    double ops_sec = stats.ops_per_second();
    EXPECT_GE(ops_sec, 0.0);
}

TEST(ThroughputStatsTest, StartResetsOpsCount) {
    ThroughputStats stats;
    stats.start();
    stats.record_ops(50);
    stats.stop();
    EXPECT_EQ(stats.total_ops(), 50u);

    // Restart resets ops
    stats.start();
    EXPECT_EQ(stats.total_ops(), 0u);
    stats.record_ops(10);
    stats.stop();
    EXPECT_EQ(stats.total_ops(), 10u);
}
