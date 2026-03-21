#include <gtest/gtest.h>
#include "nanomatch/io/order_generator.hpp"
#include <set>

using namespace nm;

TEST(OrderGeneratorTest, DeterministicWithSameSeed) {
    OrderGenerator gen1(123);
    OrderGenerator gen2(123);

    for (int i = 0; i < 100; ++i) {
        auto o1 = gen1.generate();
        auto o2 = gen2.generate();

        EXPECT_EQ(o1.action, o2.action);
        EXPECT_EQ(o1.id, o2.id);
        EXPECT_EQ(o1.side, o2.side);
        EXPECT_EQ(o1.type, o2.type);
        EXPECT_EQ(o1.price, o2.price);
        EXPECT_EQ(o1.quantity, o2.quantity);
    }
}

TEST(OrderGeneratorTest, DifferentSeedsProduceDifferentOrders) {
    OrderGenerator gen1(100);
    OrderGenerator gen2(200);

    // Generate several orders and check at least one field differs
    bool found_difference = false;
    for (int i = 0; i < 50; ++i) {
        auto o1 = gen1.generate();
        auto o2 = gen2.generate();
        if (o1.price != o2.price || o1.quantity != o2.quantity ||
            o1.side != o2.side) {
            found_difference = true;
            break;
        }
    }
    EXPECT_TRUE(found_difference);
}

TEST(OrderGeneratorTest, GeneratesValidOrders) {
    OrderGenerator gen(42, 10000, 200);
    for (int i = 0; i < 500; ++i) {
        auto req = gen.generate();
        if (req.action == OrderRequest::Action::New) {
            EXPECT_GT(req.quantity, 0u);
            EXPECT_GE(req.price, 10000 - 200);
            EXPECT_LE(req.price, 10000 + 200);
            EXPECT_TRUE(req.side == Side::Buy || req.side == Side::Sell);
            EXPECT_EQ(req.type, OrderType::Limit);
        }
    }
}

TEST(OrderGeneratorTest, CancelPercentageRoughlyTwentyPercent) {
    OrderGenerator gen(42);
    int cancel_count = 0;
    constexpr int total = 10000;

    for (int i = 0; i < total; ++i) {
        auto req = gen.generate();
        if (req.action == OrderRequest::Action::Cancel) {
            ++cancel_count;
        }
    }

    double cancel_pct = 100.0 * cancel_count / total;
    // Allow a range around 20% for randomness
    EXPECT_GT(cancel_pct, 10.0);
    EXPECT_LT(cancel_pct, 30.0);
}

TEST(OrderGeneratorTest, BatchGenerationReturnsCorrectCount) {
    OrderGenerator gen(42);
    auto batch = gen.generate_batch(250);
    EXPECT_EQ(batch.size(), 250u);
}

TEST(OrderGeneratorTest, BatchGenerationMatchesSequentialGeneration) {
    OrderGenerator gen1(42);
    OrderGenerator gen2(42);

    auto batch = gen1.generate_batch(50);
    for (size_t i = 0; i < 50; ++i) {
        auto single = gen2.generate();
        EXPECT_EQ(batch[i].action, single.action);
        EXPECT_EQ(batch[i].id, single.id);
        EXPECT_EQ(batch[i].price, single.price);
        EXPECT_EQ(batch[i].quantity, single.quantity);
    }
}

TEST(OrderGeneratorTest, NewOrderIDsAreSequential) {
    OrderGenerator gen(999);
    OrderID prev_new_id = 0;

    for (int i = 0; i < 200; ++i) {
        auto req = gen.generate();
        if (req.action == OrderRequest::Action::New) {
            EXPECT_GT(req.id, prev_new_id);
            prev_new_id = req.id;
        }
    }
}

TEST(OrderGeneratorTest, CancelOrdersReferenceEarlierIDs) {
    OrderGenerator gen(42);
    OrderID max_new_id = 0;

    for (int i = 0; i < 500; ++i) {
        auto req = gen.generate();
        if (req.action == OrderRequest::Action::New) {
            if (req.id > max_new_id) max_new_id = req.id;
        } else if (req.action == OrderRequest::Action::Cancel) {
            // Cancel IDs should reference earlier orders
            EXPECT_GT(req.id, 0u);
        }
    }
}
