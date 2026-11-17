#include "nanomatch/book/order_book.hpp"

#include <gtest/gtest.h>

using namespace nm;

class MatchingMarketTest : public ::testing::Test {
protected:
    OrderBook book{"AAPL"};
    OrderID next_id = 1;

    Order make_order(Side side, Price price, Quantity qty, OrderType type = OrderType::Limit) {
        Order o;
        o.id = next_id++;
        o.side = side;
        o.type = type;
        o.price = price;
        o.initial_quantity = qty;
        o.remaining_quantity = qty;
        return o;
    }
};

TEST_F(MatchingMarketTest, MarketBuyMatchesBestAsk) {
    book.add_order(make_order(Side::Sell, 10100, 100));
    book.add_order(make_order(Side::Sell, 10200, 200));

    auto trades = book.add_order(make_order(Side::Buy, 0, 150, OrderType::Market));

    ASSERT_EQ(trades.size(), 2u);
    EXPECT_EQ(trades[0].price, 10100);
    EXPECT_EQ(trades[0].quantity, 100u);
    EXPECT_EQ(trades[1].price, 10200);
    EXPECT_EQ(trades[1].quantity, 50u);
}

TEST_F(MatchingMarketTest, MarketSellMatchesBestBid) {
    book.add_order(make_order(Side::Buy, 10200, 100));
    book.add_order(make_order(Side::Buy, 10100, 100));

    auto trades = book.add_order(make_order(Side::Sell, 0, 150, OrderType::Market));

    ASSERT_EQ(trades.size(), 2u);
    EXPECT_EQ(trades[0].price, 10200);
    EXPECT_EQ(trades[0].quantity, 100u);
    EXPECT_EQ(trades[1].price, 10100);
    EXPECT_EQ(trades[1].quantity, 50u);
}

TEST_F(MatchingMarketTest, MarketOrderDoesNotRestOnBook) {
    // No opposing orders — market order should not rest
    auto trades = book.add_order(make_order(Side::Buy, 0, 100, OrderType::Market));

    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(book.total_orders(), 0u);  // Market orders don't rest
}

TEST_F(MatchingMarketTest, PartialMarketFillCancelsRemainder) {
    book.add_order(make_order(Side::Sell, 10000, 50));

    auto trades = book.add_order(make_order(Side::Buy, 0, 200, OrderType::Market));

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].quantity, 50u);
    EXPECT_EQ(book.total_orders(), 0u);  // Remaining 150 cancelled
}
