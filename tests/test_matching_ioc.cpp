#include "nanomatch/book/order_book.hpp"

#include <gtest/gtest.h>

using namespace nm;

class IOCTest : public ::testing::Test {
protected:
    OrderBook book{"TSLA"};
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

TEST_F(IOCTest, PartialFillCancelsRemainder) {
    book.add_order(make_order(Side::Sell, 20000, 100));

    auto trades = book.add_order(make_order(Side::Buy, 20000, 300, OrderType::ImmediateOrCancel));

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].quantity, 100u);
    EXPECT_EQ(book.total_orders(), 0u);  // Nothing rests
}

TEST_F(IOCTest, FullFill) {
    book.add_order(make_order(Side::Sell, 20000, 300));

    auto trades = book.add_order(make_order(Side::Buy, 20000, 100, OrderType::ImmediateOrCancel));

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].quantity, 100u);
    EXPECT_EQ(book.total_orders(), 1u);  // 200 remaining on ask
}

TEST_F(IOCTest, NoMatchCancelsEntireOrder) {
    auto trades = book.add_order(make_order(Side::Buy, 20000, 100, OrderType::ImmediateOrCancel));

    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(book.total_orders(), 0u);
}

TEST_F(IOCTest, IOCSellPartialFill) {
    book.add_order(make_order(Side::Buy, 15000, 200));

    auto trades = book.add_order(make_order(Side::Sell, 15000, 500, OrderType::ImmediateOrCancel));

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].quantity, 200u);
    EXPECT_EQ(book.total_orders(), 0u);  // Remainder cancelled
}
