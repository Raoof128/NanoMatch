#include "nanomatch/book/book_side.hpp"

#include <gtest/gtest.h>

using namespace nm;

class BidSideTest : public ::testing::Test {
protected:
    BidSide bids;

    Order make_buy(OrderID id, Price price, Quantity qty) {
        Order o;
        o.id = id;
        o.side = Side::Buy;
        o.type = OrderType::Limit;
        o.price = price;
        o.initial_quantity = qty;
        o.remaining_quantity = qty;
        return o;
    }
};

TEST_F(BidSideTest, EmptyOnCreate) {
    EXPECT_TRUE(bids.empty());
    EXPECT_EQ(bids.best_price(), std::nullopt);
    EXPECT_EQ(bids.level_count(), 0u);
}

TEST_F(BidSideTest, AddOrder) {
    bids.add_order(make_buy(1, 10000, 100));
    EXPECT_FALSE(bids.empty());
    EXPECT_EQ(bids.best_price(), 10000);
    EXPECT_EQ(bids.level_count(), 1u);
}

TEST_F(BidSideTest, BestPriceIsHighest) {
    bids.add_order(make_buy(1, 10000, 100));
    bids.add_order(make_buy(2, 10200, 100));
    bids.add_order(make_buy(3, 10100, 100));

    EXPECT_EQ(bids.best_price(), 10200);  // Highest price is best for bids
}

TEST_F(BidSideTest, RemoveOrderCleansLevel) {
    auto it = bids.add_order(make_buy(1, 10000, 100));
    EXPECT_EQ(bids.level_count(), 1u);

    bids.remove_order(10000, it);
    EXPECT_TRUE(bids.empty());
    EXPECT_EQ(bids.level_count(), 0u);
}

TEST_F(BidSideTest, MultipleOrdersSameLevel) {
    bids.add_order(make_buy(1, 10000, 100));
    bids.add_order(make_buy(2, 10000, 200));

    EXPECT_EQ(bids.level_count(), 1u);
    EXPECT_EQ(bids.best_level().total_quantity(), 300u);
}

class AskSideTest : public ::testing::Test {
protected:
    AskSide asks;

    Order make_sell(OrderID id, Price price, Quantity qty) {
        Order o;
        o.id = id;
        o.side = Side::Sell;
        o.type = OrderType::Limit;
        o.price = price;
        o.initial_quantity = qty;
        o.remaining_quantity = qty;
        return o;
    }
};

TEST_F(AskSideTest, BestPriceIsLowest) {
    asks.add_order(make_sell(1, 10200, 100));
    asks.add_order(make_sell(2, 10000, 100));
    asks.add_order(make_sell(3, 10100, 100));

    EXPECT_EQ(asks.best_price(), 10000);  // Lowest price is best for asks
}
