#include <gtest/gtest.h>
#include "nanomatch/book/order_book.hpp"
#include "nanomatch/engine/matching_engine.hpp"
#include "nanomatch/engine/order_validator.hpp"

using namespace nm;

class EdgeCaseTest : public ::testing::Test {
protected:
    OrderBook book{"EDGE"};
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

TEST_F(EdgeCaseTest, SingleShareOrder) {
    book.add_order(make_order(Side::Sell, 10000, 1));
    auto trades = book.add_order(make_order(Side::Buy, 10000, 1));

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].quantity, 1u);
    EXPECT_EQ(book.total_orders(), 0u);
}

TEST_F(EdgeCaseTest, LargeQuantity) {
    Quantity large = 1'000'000;
    book.add_order(make_order(Side::Sell, 10000, large));
    auto trades = book.add_order(make_order(Side::Buy, 10000, large));

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].quantity, large);
}

TEST_F(EdgeCaseTest, ManyPriceLevels) {
    // Add orders at 100 different price levels
    for (int i = 0; i < 100; ++i) {
        book.add_order(make_order(Side::Sell, 10000 + i * 10, 100));
    }
    EXPECT_EQ(book.ask_levels(), 100u);
    EXPECT_EQ(book.total_orders(), 100u);
    EXPECT_EQ(book.best_ask(), 10000);
}

TEST_F(EdgeCaseTest, DoubleCancelSameOrder) {
    auto order = make_order(Side::Buy, 10000, 100);
    OrderID id = order.id;
    book.add_order(order);

    EXPECT_TRUE(book.cancel_order(id));
    EXPECT_FALSE(book.cancel_order(id));  // Already cancelled
}

TEST_F(EdgeCaseTest, SweepEntireBook) {
    // 5 asks with 100 each = 500 total
    for (int i = 0; i < 5; ++i) {
        book.add_order(make_order(Side::Sell, 10000 + i * 100, 100));
    }

    // Buy sweeps everything
    auto trades = book.add_order(make_order(Side::Buy, 10500, 500));

    EXPECT_EQ(trades.size(), 5u);
    EXPECT_EQ(book.total_orders(), 0u);
    EXPECT_EQ(book.best_ask(), std::nullopt);
}

TEST_F(EdgeCaseTest, EmptyBookOperations) {
    EXPECT_EQ(book.best_bid(), std::nullopt);
    EXPECT_EQ(book.best_ask(), std::nullopt);
    EXPECT_EQ(book.spread(), std::nullopt);
    EXPECT_EQ(book.total_orders(), 0u);
    EXPECT_FALSE(book.cancel_order(1));
}

// Matching engine tests
TEST(MatchingEngineTest, MultiSymbol) {
    MatchingEngine engine;

    OrderRequest req1{OrderRequest::Action::New, 0, Side::Buy,
                      OrderType::Limit, 10000, 100};
    engine.process("AAPL", req1);

    OrderRequest req2{OrderRequest::Action::New, 0, Side::Sell,
                      OrderType::Limit, 20000, 100};
    engine.process("GOOG", req2);

    EXPECT_EQ(engine.book_count(), 2u);
    EXPECT_NE(engine.get_book("AAPL"), nullptr);
    EXPECT_NE(engine.get_book("GOOG"), nullptr);
    EXPECT_EQ(engine.get_book("TSLA"), nullptr);
}

TEST(MatchingEngineTest, AutoAssignsID) {
    MatchingEngine engine;

    OrderRequest req{OrderRequest::Action::New, INVALID_ID, Side::Buy,
                     OrderType::Limit, 10000, 100};
    engine.process("AAPL", req);

    auto* book = engine.get_book("AAPL");
    ASSERT_NE(book, nullptr);
    EXPECT_EQ(book->total_orders(), 1u);
}

// Validator tests
TEST(ValidatorTest, RejectsZeroQuantity) {
    OrderRequest req{OrderRequest::Action::New, 0, Side::Buy,
                     OrderType::Limit, 10000, 0};
    auto result = OrderValidator::validate(req);
    EXPECT_FALSE(result.valid);
}

TEST(ValidatorTest, RejectsNegativePrice) {
    OrderRequest req{OrderRequest::Action::New, 0, Side::Buy,
                     OrderType::Limit, -100, 100};
    auto result = OrderValidator::validate(req);
    EXPECT_FALSE(result.valid);
}

TEST(ValidatorTest, AcceptsValidOrder) {
    OrderRequest req{OrderRequest::Action::New, 0, Side::Buy,
                     OrderType::Limit, 10000, 100};
    auto result = OrderValidator::validate(req);
    EXPECT_TRUE(result.valid);
}

TEST(ValidatorTest, RejectsCancelWithInvalidID) {
    OrderRequest req{OrderRequest::Action::Cancel, INVALID_ID, Side::Buy,
                     OrderType::Limit, 0, 0};
    auto result = OrderValidator::validate(req);
    EXPECT_FALSE(result.valid);
}
