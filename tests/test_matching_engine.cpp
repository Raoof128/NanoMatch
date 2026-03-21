#include <gtest/gtest.h>
#include "nanomatch/engine/matching_engine.hpp"

using namespace nm;

class MatchingEngineTest : public ::testing::Test {
protected:
    MatchingEngine engine;

    OrderRequest new_order(Side side, Price price, Quantity qty) {
        OrderRequest req{};
        req.action = OrderRequest::Action::New;
        req.id = INVALID_ID; // let engine auto-assign
        req.side = side;
        req.type = OrderType::Limit;
        req.price = price;
        req.quantity = qty;
        return req;
    }

    OrderRequest cancel_order(OrderID id) {
        OrderRequest req{};
        req.action = OrderRequest::Action::Cancel;
        req.id = id;
        return req;
    }

    OrderRequest modify_order(OrderID id, Price price, Quantity qty) {
        OrderRequest req{};
        req.action = OrderRequest::Action::Modify;
        req.id = id;
        req.price = price;
        req.quantity = qty;
        return req;
    }
};

TEST_F(MatchingEngineTest, ProcessNewOrderCreatesBook) {
    auto trades = engine.process("AAPL", new_order(Side::Buy, 10000, 100));

    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(engine.book_count(), 1u);
    EXPECT_NE(engine.get_book("AAPL"), nullptr);
}

TEST_F(MatchingEngineTest, AutoAssignID) {
    engine.process("AAPL", new_order(Side::Buy, 10000, 100));
    engine.process("AAPL", new_order(Side::Buy, 10100, 200));

    auto* book = engine.get_book("AAPL");
    ASSERT_NE(book, nullptr);
    EXPECT_EQ(book->total_orders(), 2u);
}

TEST_F(MatchingEngineTest, ExplicitIDPreserved) {
    OrderRequest req{};
    req.action = OrderRequest::Action::New;
    req.id = 42;
    req.side = Side::Buy;
    req.type = OrderType::Limit;
    req.price = 10000;
    req.quantity = 100;

    engine.process("AAPL", req);

    auto* book = engine.get_book("AAPL");
    ASSERT_NE(book, nullptr);
    EXPECT_EQ(book->total_orders(), 1u);
}

TEST_F(MatchingEngineTest, CancelViaProcess) {
    // Add an order first; engine assigns ID 1
    engine.process("AAPL", new_order(Side::Buy, 10000, 100));

    auto* book = engine.get_book("AAPL");
    ASSERT_NE(book, nullptr);
    EXPECT_EQ(book->total_orders(), 1u);

    // Cancel order with ID 1
    auto trades = engine.process("AAPL", cancel_order(1));
    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(book->total_orders(), 0u);
}

TEST_F(MatchingEngineTest, ModifyViaProcess) {
    // Add a buy order; auto-assigned ID 1
    engine.process("AAPL", new_order(Side::Buy, 10000, 100));

    auto* book = engine.get_book("AAPL");
    ASSERT_NE(book, nullptr);

    auto old_bid = book->best_bid();
    ASSERT_TRUE(old_bid.has_value());
    EXPECT_EQ(*old_bid, 10000);

    // Modify order 1: new price 10200, new qty 50
    auto trades = engine.process("AAPL", modify_order(1, 10200, 50));

    auto new_bid = book->best_bid();
    ASSERT_TRUE(new_bid.has_value());
    EXPECT_EQ(*new_bid, 10200);
}

TEST_F(MatchingEngineTest, MultipleSymbolsIndependent) {
    engine.process("AAPL", new_order(Side::Buy, 15000, 100));
    engine.process("GOOG", new_order(Side::Sell, 280000, 50));
    engine.process("MSFT", new_order(Side::Buy, 42000, 200));

    EXPECT_EQ(engine.book_count(), 3u);

    auto* aapl = engine.get_book("AAPL");
    auto* goog = engine.get_book("GOOG");
    auto* msft = engine.get_book("MSFT");

    ASSERT_NE(aapl, nullptr);
    ASSERT_NE(goog, nullptr);
    ASSERT_NE(msft, nullptr);

    EXPECT_EQ(aapl->total_orders(), 1u);
    EXPECT_EQ(goog->total_orders(), 1u);
    EXPECT_EQ(msft->total_orders(), 1u);

    // Orders in different symbols do not interact
    EXPECT_TRUE(aapl->best_bid().has_value());
    EXPECT_FALSE(aapl->best_ask().has_value());
    EXPECT_FALSE(goog->best_bid().has_value());
    EXPECT_TRUE(goog->best_ask().has_value());
}

TEST_F(MatchingEngineTest, CrossMatchWithinSymbol) {
    // Place a resting sell order
    engine.process("AAPL", new_order(Side::Sell, 10000, 100));

    // Place a crossing buy order
    auto trades = engine.process("AAPL", new_order(Side::Buy, 10000, 100));

    EXPECT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].price, 10000);
    EXPECT_EQ(trades[0].quantity, 100u);
}

TEST_F(MatchingEngineTest, PartialCrossMatch) {
    // Resting sell for 200 shares
    engine.process("AAPL", new_order(Side::Sell, 10000, 200));

    // Buy only 100 shares
    auto trades = engine.process("AAPL", new_order(Side::Buy, 10000, 100));

    EXPECT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].quantity, 100u);

    auto* book = engine.get_book("AAPL");
    ASSERT_NE(book, nullptr);
    // 100 shares remaining on the sell side
    EXPECT_TRUE(book->best_ask().has_value());
}

TEST_F(MatchingEngineTest, GetBookReturnsNullForUnknownSymbol) {
    EXPECT_EQ(engine.get_book("UNKNOWN"), nullptr);
}

TEST_F(MatchingEngineTest, BookCountTracksCorrectly) {
    EXPECT_EQ(engine.book_count(), 0u);

    engine.process("AAPL", new_order(Side::Buy, 10000, 100));
    EXPECT_EQ(engine.book_count(), 1u);

    engine.process("GOOG", new_order(Side::Buy, 20000, 50));
    EXPECT_EQ(engine.book_count(), 2u);

    // Processing same symbol again does not increase count
    engine.process("AAPL", new_order(Side::Sell, 10100, 100));
    EXPECT_EQ(engine.book_count(), 2u);
}

TEST_F(MatchingEngineTest, NoCrossMatchAcrossSymbols) {
    // Sell in AAPL
    engine.process("AAPL", new_order(Side::Sell, 10000, 100));

    // Buy in GOOG at the same price -- should NOT match
    auto trades = engine.process("GOOG", new_order(Side::Buy, 10000, 100));
    EXPECT_TRUE(trades.empty());

    EXPECT_EQ(engine.get_book("AAPL")->total_orders(), 1u);
    EXPECT_EQ(engine.get_book("GOOG")->total_orders(), 1u);
}

TEST_F(MatchingEngineTest, CancelNonexistentOrderDoesNotCrash) {
    engine.process("AAPL", new_order(Side::Buy, 10000, 100));
    // Cancel an ID that was never placed -- should not crash
    auto trades = engine.process("AAPL", cancel_order(9999));
    EXPECT_TRUE(trades.empty());
}
