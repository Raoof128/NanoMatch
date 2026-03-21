#include <gtest/gtest.h>
#include "nanomatch/book/order_book.hpp"

using namespace nm;

class CancelModifyTest : public ::testing::Test {
protected:
    OrderBook book{"GOOG"};
    OrderID next_id = 1;

    Order make_order(Side side, Price price, Quantity qty) {
        Order o;
        o.id                = next_id++;
        o.side              = side;
        o.type              = OrderType::Limit;
        o.price             = price;
        o.initial_quantity  = qty;
        o.remaining_quantity = qty;
        return o;
    }
};

TEST_F(CancelModifyTest, CancelsExistingOrder) {
    auto order = make_order(Side::Buy, 15000, 100);
    OrderID id = order.id;
    book.add_order(order);

    EXPECT_EQ(book.total_orders(), 1u);

    bool cancelled = book.cancel_order(id);
    EXPECT_TRUE(cancelled);
    EXPECT_EQ(book.total_orders(), 0u);
    EXPECT_EQ(book.best_bid(), std::nullopt);
}

TEST_F(CancelModifyTest, CancelNonExistentReturnsFalse) {
    EXPECT_FALSE(book.cancel_order(9999));
}

TEST_F(CancelModifyTest, CancelFromMultipleOrders) {
    auto o1 = make_order(Side::Buy, 10000, 100);
    auto o2 = make_order(Side::Buy, 10000, 200);
    OrderID id1 = o1.id;
    book.add_order(o1);
    book.add_order(o2);

    EXPECT_EQ(book.total_orders(), 2u);

    book.cancel_order(id1);
    EXPECT_EQ(book.total_orders(), 1u);
    EXPECT_EQ(book.best_bid(), 10000);  // Still has o2
}

TEST_F(CancelModifyTest, CancelLastAtPriceLevelRemovesLevel) {
    book.add_order(make_order(Side::Sell, 10000, 100));
    auto o2 = make_order(Side::Sell, 10200, 100);
    OrderID id2 = o2.id;
    book.add_order(o2);

    EXPECT_EQ(book.ask_levels(), 2u);

    book.cancel_order(id2);
    EXPECT_EQ(book.ask_levels(), 1u);
    EXPECT_EQ(book.best_ask(), 10000);
}

TEST_F(CancelModifyTest, ModifyChangesPrice) {
    auto order = make_order(Side::Buy, 10000, 100);
    OrderID id = order.id;
    book.add_order(order);

    EXPECT_EQ(book.best_bid(), 10000);

    book.modify_order(id, 10500, 100);

    EXPECT_EQ(book.best_bid(), 10500);
    EXPECT_EQ(book.total_orders(), 1u);
}

TEST_F(CancelModifyTest, ModifyChangesQuantity) {
    auto order = make_order(Side::Sell, 10000, 100);
    OrderID id = order.id;
    book.add_order(order);

    book.modify_order(id, 10000, 500);

    EXPECT_EQ(book.total_orders(), 1u);
    EXPECT_EQ(book.best_ask(), 10000);
}

TEST_F(CancelModifyTest, ModifyNonExistentReturnsEmpty) {
    auto trades = book.modify_order(9999, 10000, 100);
    EXPECT_TRUE(trades.empty());
}

TEST_F(CancelModifyTest, ModifyCanTriggerMatch) {
    book.add_order(make_order(Side::Sell, 10000, 100));
    auto buy = make_order(Side::Buy, 9500, 100);
    OrderID buy_id = buy.id;
    book.add_order(buy);

    // No match yet (spread $5)
    EXPECT_EQ(book.total_orders(), 2u);

    // Modify buy price up to cross
    auto trades = book.modify_order(buy_id, 10000, 100);

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].quantity, 100u);
    EXPECT_EQ(book.total_orders(), 0u);
}
