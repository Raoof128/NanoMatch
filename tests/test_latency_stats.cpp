#include <gtest/gtest.h>
#include "nanomatch/stats/latency_stats.hpp"

using namespace nm;

TEST(LatencyStatsTest, EmptyStatsReturnZero) {
    LatencyStats stats;
    EXPECT_DOUBLE_EQ(stats.average(), 0.0);
    EXPECT_EQ(stats.min(), 0u);
    EXPECT_EQ(stats.max(), 0u);
    EXPECT_EQ(stats.percentile(50.0), 0u);
    EXPECT_EQ(stats.percentile(99.0), 0u);
    EXPECT_EQ(stats.count(), 0u);
}

TEST(LatencyStatsTest, RecordAndComputeAverage) {
    LatencyStats stats;
    stats.record(100);
    stats.record(200);
    stats.record(300);

    EXPECT_DOUBLE_EQ(stats.average(), 200.0);
}

TEST(LatencyStatsTest, SingleValueAverage) {
    LatencyStats stats;
    stats.record(500);
    EXPECT_DOUBLE_EQ(stats.average(), 500.0);
}

TEST(LatencyStatsTest, PercentileP50) {
    LatencyStats stats;
    for (uint64_t i = 1; i <= 100; ++i) {
        stats.record(i);
    }

    uint64_t p50 = stats.percentile(50.0);
    // p50 index = 50/100 * 100 = 50, clamped to size-1=99
    // sorted[50] = 51
    EXPECT_EQ(p50, 51u);
}

TEST(LatencyStatsTest, PercentileP99) {
    LatencyStats stats;
    for (uint64_t i = 1; i <= 100; ++i) {
        stats.record(i);
    }

    uint64_t p99 = stats.percentile(99.0);
    // p99 index = 99/100 * 100 = 99, which is the last element
    EXPECT_EQ(p99, 100u);
}

TEST(LatencyStatsTest, PercentileP0ReturnsMinimum) {
    LatencyStats stats;
    stats.record(500);
    stats.record(100);
    stats.record(300);

    uint64_t p0 = stats.percentile(0.0);
    EXPECT_EQ(p0, 100u);
}

TEST(LatencyStatsTest, MinTracking) {
    LatencyStats stats;
    stats.record(300);
    stats.record(100);
    stats.record(500);

    EXPECT_EQ(stats.min(), 100u);
}

TEST(LatencyStatsTest, MaxTracking) {
    LatencyStats stats;
    stats.record(300);
    stats.record(100);
    stats.record(500);

    EXPECT_EQ(stats.max(), 500u);
}

TEST(LatencyStatsTest, CountTracking) {
    LatencyStats stats;
    EXPECT_EQ(stats.count(), 0u);

    stats.record(10);
    EXPECT_EQ(stats.count(), 1u);

    stats.record(20);
    stats.record(30);
    EXPECT_EQ(stats.count(), 3u);
}

TEST(LatencyStatsTest, ReserveDoesNotAffectResults) {
    LatencyStats stats_reserved;
    LatencyStats stats_plain;

    stats_reserved.reserve(1000);

    for (uint64_t i = 1; i <= 50; ++i) {
        stats_reserved.record(i * 10);
        stats_plain.record(i * 10);
    }

    EXPECT_EQ(stats_reserved.count(), stats_plain.count());
    EXPECT_DOUBLE_EQ(stats_reserved.average(), stats_plain.average());
    EXPECT_EQ(stats_reserved.min(), stats_plain.min());
    EXPECT_EQ(stats_reserved.max(), stats_plain.max());
    EXPECT_EQ(stats_reserved.percentile(50.0), stats_plain.percentile(50.0));
    EXPECT_EQ(stats_reserved.percentile(99.0), stats_plain.percentile(99.0));
}

TEST(LatencyStatsTest, DataReturnsAllRecordedValues) {
    LatencyStats stats;
    stats.record(10);
    stats.record(20);
    stats.record(30);

    const auto& data = stats.data();
    ASSERT_EQ(data.size(), 3u);
    EXPECT_EQ(data[0], 10u);
    EXPECT_EQ(data[1], 20u);
    EXPECT_EQ(data[2], 30u);
}
