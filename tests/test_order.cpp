#include "nanomatch/core/order.hpp"

#include <gtest/gtest.h>

using namespace nm;

TEST(OrderTest, InitialState) {
    Order o;
    o.id = 1;
    o.side = Side::Buy;
    o.type = OrderType::Limit;
    o.price = 10000;
    o.initial_quantity = 100;
    o.remaining_quantity = 100;

    EXPECT_FALSE(o.is_filled());
    EXPECT_EQ(o.filled_quantity(), 0u);
}

TEST(OrderTest, PartialFill) {
    Order o;
    o.initial_quantity = 100;
    o.remaining_quantity = 100;

    o.fill(40);
    EXPECT_EQ(o.remaining_quantity, 60u);
    EXPECT_EQ(o.filled_quantity(), 40u);
    EXPECT_FALSE(o.is_filled());
}

TEST(OrderTest, CompleteFill) {
    Order o;
    o.initial_quantity = 100;
    o.remaining_quantity = 100;

    o.fill(100);
    EXPECT_EQ(o.remaining_quantity, 0u);
    EXPECT_TRUE(o.is_filled());
}

TEST(OrderTest, OverFillClamps) {
    Order o;
    o.initial_quantity = 100;
    o.remaining_quantity = 50;

    o.fill(200);  // Should clamp to remaining
    EXPECT_EQ(o.remaining_quantity, 0u);
    EXPECT_TRUE(o.is_filled());
}

TEST(OrderTest, SetMarketPriceBuy) {
    Order o;
    o.side = Side::Buy;
    o.set_market_price();
    EXPECT_EQ(o.price, MAX_PRICE);
}

TEST(OrderTest, SetMarketPriceSell) {
    Order o;
    o.side = Side::Sell;
    o.set_market_price();
    EXPECT_EQ(o.price, MIN_PRICE);
}
