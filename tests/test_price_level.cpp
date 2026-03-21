#include <gtest/gtest.h>
#include "nanomatch/core/price_level.hpp"

using namespace nm;

class PriceLevelTest : public ::testing::Test {
protected:
    PriceLevel level{10000};

    Order make_order(OrderID id, Quantity qty) {
        Order o;
        o.id = id;
        o.side = Side::Buy;
        o.type = OrderType::Limit;
        o.price = 10000;
        o.initial_quantity = qty;
        o.remaining_quantity = qty;
        return o;
    }
};

TEST_F(PriceLevelTest, EmptyOnCreate) {
    EXPECT_TRUE(level.empty());
    EXPECT_EQ(level.order_count(), 0u);
    EXPECT_EQ(level.total_quantity(), 0u);
    EXPECT_EQ(level.price(), 10000);
}

TEST_F(PriceLevelTest, AddOrder) {
    level.add_order(make_order(1, 100));
    EXPECT_FALSE(level.empty());
    EXPECT_EQ(level.order_count(), 1u);
    EXPECT_EQ(level.total_quantity(), 100u);
}

TEST_F(PriceLevelTest, MultipleOrders) {
    level.add_order(make_order(1, 100));
    level.add_order(make_order(2, 200));
    level.add_order(make_order(3, 300));

    EXPECT_EQ(level.order_count(), 3u);
    EXPECT_EQ(level.total_quantity(), 600u);
}

TEST_F(PriceLevelTest, FIFOOrder) {
    level.add_order(make_order(1, 100));
    level.add_order(make_order(2, 200));

    EXPECT_EQ(level.front().id, 1u);  // First in = first out
}

TEST_F(PriceLevelTest, RemoveOrder) {
    auto it = level.add_order(make_order(1, 100));
    level.add_order(make_order(2, 200));

    level.remove_order(it);
    EXPECT_EQ(level.order_count(), 1u);
    EXPECT_EQ(level.total_quantity(), 200u);
    EXPECT_EQ(level.front().id, 2u);
}

TEST_F(PriceLevelTest, FillFrontPartial) {
    level.add_order(make_order(1, 100));
    level.add_order(make_order(2, 200));

    Quantity filled = level.fill_front(40);
    EXPECT_EQ(filled, 40u);
    EXPECT_EQ(level.total_quantity(), 260u);  // 60 + 200
    EXPECT_EQ(level.front().remaining_quantity, 60u);
    EXPECT_EQ(level.order_count(), 2u);
}

TEST_F(PriceLevelTest, FillFrontComplete) {
    level.add_order(make_order(1, 100));
    level.add_order(make_order(2, 200));

    Quantity filled = level.fill_front(100);
    EXPECT_EQ(filled, 100u);
    EXPECT_EQ(level.total_quantity(), 200u);
    EXPECT_EQ(level.front().id, 2u);  // Order 1 removed
    EXPECT_EQ(level.order_count(), 1u);
}

TEST_F(PriceLevelTest, FillLastOrder) {
    level.add_order(make_order(1, 100));

    Quantity filled = level.fill_front(100);
    EXPECT_EQ(filled, 100u);
    EXPECT_TRUE(level.empty());
    EXPECT_EQ(level.total_quantity(), 0u);
}
