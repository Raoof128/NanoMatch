#include "nanomatch/book/order_book.hpp"

#include <gtest/gtest.h>

using namespace nm;

class MatchingLimitTest : public ::testing::Test {
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

TEST_F(MatchingLimitTest, LimitOrderRestsOnBook) {
    auto trades = book.add_order(make_order(Side::Buy, 10000, 100));

    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(book.best_bid(), 10000);
    EXPECT_EQ(book.best_ask(), std::nullopt);
    EXPECT_EQ(book.total_orders(), 1u);
}

TEST_F(MatchingLimitTest, CrossingOrderGeneratesTrade) {
    book.add_order(make_order(Side::Sell, 10000, 100));

    auto trades = book.add_order(make_order(Side::Buy, 10000, 100));

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].price, 10000);
    EXPECT_EQ(trades[0].quantity, 100u);
    EXPECT_EQ(book.total_orders(), 0u);
}

TEST_F(MatchingLimitTest, PartialFillLeavesRemainder) {
    book.add_order(make_order(Side::Sell, 10100, 500));

    auto trades = book.add_order(make_order(Side::Buy, 10100, 200));

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].quantity, 200u);
    EXPECT_EQ(book.total_orders(), 1u);
}

TEST_F(MatchingLimitTest, PriceTimePriority) {
    auto sell1 = make_order(Side::Sell, 10000, 100);
    auto sell2 = make_order(Side::Sell, 10000, 100);
    OrderID sell1_id = sell1.id;
    book.add_order(sell1);
    book.add_order(sell2);

    auto trades = book.add_order(make_order(Side::Buy, 10000, 100));

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].sell_order_id, sell1_id);  // FIFO
}

TEST_F(MatchingLimitTest, PricePriorityAcrossLevels) {
    book.add_order(make_order(Side::Sell, 10200, 100));  // $102.00
    book.add_order(make_order(Side::Sell, 10100, 100));  // $101.00 ← better

    auto trades = book.add_order(make_order(Side::Buy, 10200, 100));

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].price, 10100);  // Matched at better price
}

TEST_F(MatchingLimitTest, MultipleFillsAcrossLevels) {
    book.add_order(make_order(Side::Sell, 10100, 100));
    book.add_order(make_order(Side::Sell, 10200, 200));

    auto trades = book.add_order(make_order(Side::Buy, 10200, 250));

    ASSERT_EQ(trades.size(), 2u);
    EXPECT_EQ(trades[0].price, 10100);
    EXPECT_EQ(trades[0].quantity, 100u);
    EXPECT_EQ(trades[1].price, 10200);
    EXPECT_EQ(trades[1].quantity, 150u);
}

TEST_F(MatchingLimitTest, NoMatchWhenPricesDontCross) {
    book.add_order(make_order(Side::Sell, 10200, 100));               // Ask at $102
    auto trades = book.add_order(make_order(Side::Buy, 10100, 100));  // Bid at $101

    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(book.best_bid(), 10100);
    EXPECT_EQ(book.best_ask(), 10200);
    EXPECT_EQ(book.total_orders(), 2u);
}

TEST_F(MatchingLimitTest, BuyerGetsImprovedPrice) {
    book.add_order(make_order(Side::Sell, 10000, 100));  // Sell at $100

    // Buy at $105 — should match at resting price $100 (price improvement)
    auto trades = book.add_order(make_order(Side::Buy, 10500, 100));

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].price, 10000);  // Executed at resting order's price
}

TEST_F(MatchingLimitTest, SellSweepsMultipleBids) {
    book.add_order(make_order(Side::Buy, 10200, 100));
    book.add_order(make_order(Side::Buy, 10100, 100));
    book.add_order(make_order(Side::Buy, 10000, 100));

    auto trades = book.add_order(make_order(Side::Sell, 10000, 250));

    ASSERT_EQ(trades.size(), 3u);
    EXPECT_EQ(trades[0].price, 10200);  // Best bid first
    EXPECT_EQ(trades[0].quantity, 100u);
    EXPECT_EQ(trades[1].price, 10100);
    EXPECT_EQ(trades[1].quantity, 100u);
    EXPECT_EQ(trades[2].price, 10000);
    EXPECT_EQ(trades[2].quantity, 50u);
    EXPECT_EQ(book.total_orders(), 1u);  // 50 shares remain on bid at $100
}

TEST_F(MatchingLimitTest, TradeRecordsBuyAndSellIDs) {
    auto sell = make_order(Side::Sell, 10000, 100);
    OrderID sell_id = sell.id;
    book.add_order(sell);

    auto buy = make_order(Side::Buy, 10000, 100);
    OrderID buy_id = buy.id;
    auto trades = book.add_order(buy);

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].buy_order_id, buy_id);
    EXPECT_EQ(trades[0].sell_order_id, sell_id);
}
