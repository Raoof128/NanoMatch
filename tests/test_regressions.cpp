// Regression tests for defects found in the November 2026 correctness review.
//
// Each test uses only the API that existed before the fixes, so it can be run against the
// old code. All of them failed there except the three marked "Pins existing behaviour",
// which passed before and after and are here to keep it that way.

#include "nanomatch/book/order_book.hpp"
#include "nanomatch/core/types.hpp"
#include "nanomatch/engine/matching_engine.hpp"
#include "nanomatch/io/csv_reader.hpp"
#include "nanomatch/memory/pool_allocator.hpp"

#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

using namespace nm;

namespace {

Order make_order(OrderID id, Side side, Price price, Quantity qty,
                 OrderType type = OrderType::Limit) {
    Order o{};
    o.id = id;
    o.side = side;
    o.type = type;
    o.price = price;
    o.initial_quantity = qty;
    o.remaining_quantity = qty;
    return o;
}

OrderRequest new_request(OrderID id, Side side, Price price, Quantity qty) {
    return OrderRequest{OrderRequest::Action::New, id, side, OrderType::Limit, price, qty};
}

}  // namespace

// A second resting order with a live ID used to overwrite the lookup entry, leaving the
// first order on the book where no cancel could reach it.
TEST(RegressionTest, DuplicateLiveIdIsRejectedInsteadOfOrphaningTheFirstOrder) {
    OrderBook book("X");
    book.add_order(make_order(7, Side::Buy, 10000, 100));
    book.add_order(make_order(7, Side::Buy, 10100, 50));  // same ID while the first rests

    EXPECT_EQ(book.total_orders(), 1u);
    EXPECT_EQ(book.best_bid(), 10000);  // the original order, untouched
    EXPECT_EQ(book.bid_levels(), 1u);

    EXPECT_TRUE(book.cancel_order(7));
    EXPECT_EQ(book.best_bid(), std::nullopt);  // nothing left behind
    EXPECT_EQ(book.bid_levels(), 0u);
}

// A price level summed its quantity in 32 bits, so two large orders wrapped the total and a
// fill-or-kill order that the level could cover was turned away.
TEST(RegressionTest, LevelQuantityDoesNotWrapAtThirtyTwoBits) {
    OrderBook book("X");
    const Quantity big = 3'000'000'000u;
    book.add_order(make_order(1, Side::Sell, 10000, big));
    book.add_order(make_order(2, Side::Sell, 10000, big));

    auto trades =
        book.add_order(make_order(3, Side::Buy, 10000, 4'000'000'000u, OrderType::FillOrKill));

    ASSERT_EQ(trades.size(), 2u);
    EXPECT_EQ(trades[0].quantity, big);
    EXPECT_EQ(trades[1].quantity, 1'000'000'000u);
    EXPECT_EQ(book.total_orders(), 1u);  // order 2 keeps its remaining 2,000,000,000
}

// Negating the most negative price is undefined behaviour in 32 bits.
TEST(RegressionTest, PriceToStringHandlesTheMostNegativePrice) {
    EXPECT_EQ(price_to_string(MIN_PRICE), "-21474836.48");
    EXPECT_EQ(price_to_string(MAX_PRICE), "21474836.47");
    EXPECT_EQ(price_to_string(-5), "-0.05");
    EXPECT_EQ(price_to_string(10150), "101.50");
}

// A limit order priced at zero or below used to rest on the book.
TEST(RegressionTest, LimitOrderWithNonPositivePriceIsRejected) {
    OrderBook book("X");
    book.add_order(make_order(1, Side::Buy, 0, 100));
    book.add_order(make_order(2, Side::Buy, -500, 100));

    EXPECT_EQ(book.total_orders(), 0u);
    EXPECT_EQ(book.best_bid(), std::nullopt);
}

// Reducing an order's quantity at the same price used to send it to the back of the queue.
TEST(RegressionTest, ReducingQuantityAtTheSamePriceKeepsTimePriority) {
    OrderBook book("X");
    book.add_order(make_order(1, Side::Buy, 10000, 100));
    book.add_order(make_order(2, Side::Buy, 10000, 100));

    book.modify_order(1, 10000, 60);  // smaller, same price

    auto trades = book.add_order(make_order(3, Side::Sell, 10000, 60));
    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].buy_order_id, 1u);  // order 1 is still first in the queue
    EXPECT_EQ(trades[0].quantity, 60u);
}

// Pins existing behaviour: increasing quantity, or changing price, is a new order for
// queueing purposes.
TEST(RegressionTest, IncreasingQuantityLosesTimePriority) {
    OrderBook book("X");
    book.add_order(make_order(1, Side::Buy, 10000, 100));
    book.add_order(make_order(2, Side::Buy, 10000, 100));

    book.modify_order(1, 10000, 150);

    auto trades = book.add_order(make_order(3, Side::Sell, 10000, 100));
    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].buy_order_id, 2u);
}

// A modify to zero quantity or a non-positive price used to delete the order silently.
TEST(RegressionTest, InvalidModifyLeavesTheOrderUntouched) {
    OrderBook book("X");
    book.add_order(make_order(1, Side::Sell, 10000, 100));

    EXPECT_TRUE(book.modify_order(1, 10000, 0).empty());
    EXPECT_TRUE(book.modify_order(1, 0, 100).empty());
    EXPECT_TRUE(book.modify_order(1, -1, 100).empty());

    EXPECT_EQ(book.total_orders(), 1u);
    EXPECT_EQ(book.best_ask(), 10000);
    auto trades = book.add_order(make_order(2, Side::Buy, 10000, 100));
    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].quantity, 100u);  // the original quantity is intact
}

// The engine handed out IDs from its own counter without looking at IDs callers had supplied,
// so an automatic ID could collide with a live one.
TEST(RegressionTest, AutomaticIdsDoNotCollideWithCallerSuppliedIds) {
    MatchingEngine engine;
    engine.process("X", new_request(1, Side::Buy, 10000, 100));           // caller picks 1
    engine.process("X", new_request(INVALID_ID, Side::Buy, 10100, 100));  // engine picks

    auto* book = engine.get_book("X");
    ASSERT_NE(book, nullptr);
    EXPECT_EQ(book->total_orders(), 2u);
    EXPECT_EQ(book->bid_levels(), 2u);
    EXPECT_TRUE(book->cancel_order(1));
    EXPECT_EQ(book->total_orders(), 1u);
    EXPECT_EQ(book->bid_levels(), 1u);
}

// A cancel or modify for a symbol the engine had never seen used to create an empty book.
TEST(RegressionTest, CancelForAnUnknownSymbolDoesNotCreateABook) {
    MatchingEngine engine;
    engine.process(
        "GHOST", OrderRequest{OrderRequest::Action::Cancel, 5, Side::Buy, OrderType::Limit, 0, 0});
    engine.process("GHOST2", OrderRequest{OrderRequest::Action::Modify, 5, Side::Buy,
                                          OrderType::Limit, 10000, 10});

    EXPECT_EQ(engine.book_count(), 0u);
    EXPECT_EQ(engine.get_book("GHOST"), nullptr);
}

// The engine did not apply the validator, so a request it documents as invalid went through.
TEST(RegressionTest, EngineRejectsRequestsTheValidatorRejects) {
    MatchingEngine engine;
    engine.process("X", new_request(INVALID_ID, Side::Buy, -100, 10));

    auto* book = engine.get_book("X");
    EXPECT_TRUE(book == nullptr || book->total_orders() == 0u);
}

// Returning the same slot twice used to corrupt the free list: the pool then handed one slot
// to two callers.
TEST(RegressionTest, PoolAllocatorRejectsDoubleFree) {
    PoolAllocator<int> pool(4);
    int* a = pool.allocate();
    ASSERT_NE(a, nullptr);
    pool.deallocate(a);

    EXPECT_THROW(pool.deallocate(a), std::logic_error);
    EXPECT_EQ(pool.size(), 0u);

    int* b = pool.allocate();
    int* c = pool.allocate();
    EXPECT_NE(b, c);
}

TEST(RegressionTest, PoolAllocatorRejectsASlotThatWasNeverAllocated) {
    PoolAllocator<int> pool(4);
    int* a = pool.allocate();
    ASSERT_NE(a, nullptr);

    EXPECT_THROW(pool.deallocate(a + 1), std::logic_error);  // inside the pool, never handed out
    EXPECT_EQ(pool.size(), 1u);
}

// ── CSV reader ──

class CsvRegressionTest : public ::testing::Test {
protected:
    std::string path_;

    void SetUp() override {
        path_ = "/tmp/nanomatch_regression_" +
                std::to_string(::testing::UnitTest::GetInstance()->current_test_info()->line()) +
                ".csv";
    }
    void TearDown() override {
        std::remove(path_.c_str());
    }
    std::vector<OrderRequest> read(const std::string& body) {
        {
            std::ofstream f(path_);
            f << "action,id,side,type,price,quantity\n" << body;
        }
        return CSVReader(path_).read_orders();
    }
};

// Unknown words used to be read as NEW, SELL and LIMIT.
TEST_F(CsvRegressionTest, UnknownActionIsAnError) {
    EXPECT_THROW(read("BUYNOW,1,BUY,LIMIT,10000,10\n"), std::runtime_error);
}

TEST_F(CsvRegressionTest, UnknownSideIsAnError) {
    EXPECT_THROW(read("NEW,1,LONG,LIMIT,10000,10\n"), std::runtime_error);
}

TEST_F(CsvRegressionTest, UnknownOrderTypeIsAnError) {
    EXPECT_THROW(read("NEW,1,BUY,STOP,10000,10\n"), std::runtime_error);
}

// "-5" used to wrap to 4,294,967,291 shares.
TEST_F(CsvRegressionTest, NegativeQuantityIsAnError) {
    EXPECT_THROW(read("NEW,1,BUY,LIMIT,10000,-5\n"), std::runtime_error);
}

// "100abc" used to be read as 100, and an out-of-range price was truncated.
TEST_F(CsvRegressionTest, TrailingJunkAndOutOfRangeNumbersAreErrors) {
    EXPECT_THROW(read("NEW,1,BUY,LIMIT,100abc,10\n"), std::runtime_error);
    EXPECT_THROW(read("NEW,1,BUY,LIMIT,99999999999,10\n"), std::runtime_error);
    EXPECT_THROW(read("NEW,1,BUY,LIMIT,10000,99999999999\n"), std::runtime_error);
}

// Pins existing behaviour.
TEST_F(CsvRegressionTest, ErrorNamesTheLine) {
    try {
        read("NEW,1,BUY,LIMIT,10000,10\nNEW,2,BUY,LIMIT,10000,oops\n");
        FAIL() << "expected a parse error";
    } catch (const std::runtime_error& e) {
        EXPECT_NE(std::string(e.what()).find("Line 3"), std::string::npos) << e.what();
    }
}

// Pins existing behaviour: the old reader took "10\r" as 10 only because it ignored
// whatever followed a number. The strict reader has to strip the carriage return itself.
TEST_F(CsvRegressionTest, WindowsLineEndingsAreAccepted) {
    auto orders = read("NEW,1,BUY,LIMIT,10000,10\r\nCANCEL,1,BUY,LIMIT,0,0\r\n");
    ASSERT_EQ(orders.size(), 2u);
    EXPECT_EQ(orders[0].quantity, 10u);
    EXPECT_EQ(orders[1].action, OrderRequest::Action::Cancel);
}

// A cancel needs only its ID, but the old reader demanded a price and a quantity.
TEST_F(CsvRegressionTest, CancelRowMayLeaveUnusedColumnsEmpty) {
    auto orders = read("CANCEL,7,,,,\n");
    ASSERT_EQ(orders.size(), 1u);
    EXPECT_EQ(orders[0].action, OrderRequest::Action::Cancel);
    EXPECT_EQ(orders[0].id, 7u);
}

// A row with a column missing used to be read with whatever came next in its place.
TEST_F(CsvRegressionTest, WrongColumnCountIsAnError) {
    EXPECT_THROW(read("NEW,1,BUY,LIMIT,10000\n"), std::runtime_error);
    EXPECT_THROW(read("NEW,1,BUY,LIMIT,10000,10,extra\n"), std::runtime_error);
}

// The first line was always thrown away, so a file with no header lost its first order.
TEST_F(CsvRegressionTest, FileWithoutAHeaderKeepsItsFirstRow) {
    {
        std::ofstream f(path_);
        f << "NEW,1,BUY,LIMIT,10000,10\nNEW,2,SELL,LIMIT,10100,5\n";
    }
    auto orders = CSVReader(path_).read_orders();
    ASSERT_EQ(orders.size(), 2u);
    EXPECT_EQ(orders[0].id, 1u);
}
