#include <gtest/gtest.h>
#include "nanomatch/book/order_book.hpp"

using namespace nm;

class FOKTest : public ::testing::Test {
protected:
    OrderBook book{"MSFT"};
    OrderID next_id = 1;

    Order make_order(Side side, Price price, Quantity qty,
                     OrderType type = OrderType::Limit) {
        Order o;
        o.id                = next_id++;
        o.side              = side;
        o.type              = type;
        o.price             = price;
        o.initial_quantity  = qty;
        o.remaining_quantity = qty;
        return o;
    }
};

TEST_F(FOKTest, RejectsWhenCantFillCompletely) {
    book.add_order(make_order(Side::Sell, 30000, 100));

    auto trades = book.add_order(
        make_order(Side::Buy, 30000, 500, OrderType::FillOrKill));

    EXPECT_TRUE(trades.empty());      // Rejected
    EXPECT_EQ(book.total_orders(), 1u); // Resting sell still there
}

TEST_F(FOKTest, FillsWhenExactMatch) {
    book.add_order(make_order(Side::Sell, 30000, 100));

    auto trades = book.add_order(
        make_order(Side::Buy, 30000, 100, OrderType::FillOrKill));

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].quantity, 100u);
    EXPECT_EQ(book.total_orders(), 0u);
}

TEST_F(FOKTest, FillsAcrossMultipleLevels) {
    book.add_order(make_order(Side::Sell, 30000, 100));
    book.add_order(make_order(Side::Sell, 30100, 100));

    auto trades = book.add_order(
        make_order(Side::Buy, 30100, 200, OrderType::FillOrKill));

    ASSERT_EQ(trades.size(), 2u);
    EXPECT_EQ(trades[0].quantity, 100u);
    EXPECT_EQ(trades[1].quantity, 100u);
    EXPECT_EQ(book.total_orders(), 0u);
}

TEST_F(FOKTest, RejectsWhenNoOpposingSide) {
    auto trades = book.add_order(
        make_order(Side::Buy, 30000, 100, OrderType::FillOrKill));

    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(book.total_orders(), 0u);
}

TEST_F(FOKTest, FOKSellRejectsInsufficientBids) {
    book.add_order(make_order(Side::Buy, 25000, 50));

    auto trades = book.add_order(
        make_order(Side::Sell, 25000, 100, OrderType::FillOrKill));

    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(book.total_orders(), 1u);
}
