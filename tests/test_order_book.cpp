#include "nanomatch/book/order_book.hpp"

#include <gtest/gtest.h>

using namespace nm;

TEST(OrderBookTest, CreateWithSymbol) {
    OrderBook book("AAPL");
    EXPECT_EQ(book.symbol(), "AAPL");
    EXPECT_EQ(book.total_orders(), 0u);
    EXPECT_EQ(book.best_bid(), std::nullopt);
    EXPECT_EQ(book.best_ask(), std::nullopt);
    EXPECT_EQ(book.spread(), std::nullopt);
}

TEST(OrderBookTest, BidAskAfterOrders) {
    OrderBook book("GOOG");

    Order buy;
    buy.id = 1;
    buy.side = Side::Buy;
    buy.type = OrderType::Limit;
    buy.price = 10000;
    buy.initial_quantity = 100;
    buy.remaining_quantity = 100;
    book.add_order(buy);

    Order sell;
    sell.id = 2;
    sell.side = Side::Sell;
    sell.type = OrderType::Limit;
    sell.price = 10200;
    sell.initial_quantity = 100;
    sell.remaining_quantity = 100;
    book.add_order(sell);

    EXPECT_EQ(book.best_bid(), 10000);
    EXPECT_EQ(book.best_ask(), 10200);
    EXPECT_EQ(book.spread(), 200);  // $2.00
    EXPECT_EQ(book.total_orders(), 2u);
}

TEST(OrderBookTest, SpreadNarrowsOnNewOrder) {
    OrderBook book("TSLA");

    Order buy;
    buy.id = 1;
    buy.side = Side::Buy;
    buy.type = OrderType::Limit;
    buy.price = 10000;
    buy.initial_quantity = 100;
    buy.remaining_quantity = 100;
    book.add_order(buy);

    Order sell;
    sell.id = 2;
    sell.side = Side::Sell;
    sell.type = OrderType::Limit;
    sell.price = 10500;
    sell.initial_quantity = 100;
    sell.remaining_quantity = 100;
    book.add_order(sell);

    EXPECT_EQ(book.spread(), 500);  // $5.00

    // New buy closer to ask
    Order buy2;
    buy2.id = 3;
    buy2.side = Side::Buy;
    buy2.type = OrderType::Limit;
    buy2.price = 10300;
    buy2.initial_quantity = 50;
    buy2.remaining_quantity = 50;
    book.add_order(buy2);

    EXPECT_EQ(book.spread(), 200);  // Narrowed to $2.00
}
