// Randomized differential test with fixed seeds.
//
// A stream of random requests (valid and invalid) is applied to the real OrderBook and to
// a deliberately naive reference book written here: one flat vector, linear scans, no
// shared code with the real one. After every request the two must agree on the trades
// produced and on the resting orders, and the real book must pass its own consistency
// check. Price-time priority is covered by the trade comparison, because the reference
// picks the resting order to fill by best price and then by arrival sequence.
//
// The seeds are fixed, so a failure reproduces exactly.

#include "nanomatch/book/order_book.hpp"

#include <algorithm>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace nm;

namespace {

struct RefOrder {
    OrderID id;
    Side side;
    Price price;
    Quantity remaining;
    uint64_t sequence;  // arrival order: lower means earlier
};

struct RefTrade {
    OrderID buy_id;
    OrderID sell_id;
    Price price;
    Quantity quantity;
};

class ReferenceBook {
public:
    std::vector<RefTrade> add(OrderID id, Side side, OrderType type, Price price, Quantity qty) {
        std::vector<RefTrade> trades;
        if (id == INVALID_ID || qty == 0)
            return trades;
        if (type == OrderType::Limit && price <= 0)
            return trades;
        if (find(id) != resting_.end())
            return trades;

        if (type == OrderType::Market)
            price = (side == Side::Buy) ? MAX_PRICE : MIN_PRICE;

        if (type == OrderType::FillOrKill) {
            uint64_t available = 0;
            for (const auto& o : resting_) {
                if (o.side != side && crosses(side, price, o.price))
                    available += o.remaining;
            }
            if (available < qty)
                return trades;
        }

        Quantity remaining = qty;
        while (remaining > 0) {
            auto best = resting_.end();
            for (auto it = resting_.begin(); it != resting_.end(); ++it) {
                if (it->side == side || !crosses(side, price, it->price))
                    continue;
                if (best == resting_.end() || better(*it, *best))
                    best = it;
            }
            if (best == resting_.end())
                break;

            const Quantity fill = std::min(remaining, best->remaining);
            if (side == Side::Buy)
                trades.push_back({id, best->id, best->price, fill});
            else
                trades.push_back({best->id, id, best->price, fill});
            remaining -= fill;
            best->remaining -= fill;
            if (best->remaining == 0)
                resting_.erase(best);
        }

        if (remaining > 0 && type == OrderType::Limit)
            resting_.push_back({id, side, price, remaining, next_sequence_++});
        return trades;
    }

    bool cancel(OrderID id) {
        auto it = find(id);
        if (it == resting_.end())
            return false;
        resting_.erase(it);
        return true;
    }

    std::vector<RefTrade> modify(OrderID id, Price price, Quantity qty) {
        if (qty == 0 || price <= 0)
            return {};
        auto it = find(id);
        if (it == resting_.end())
            return {};
        if (price == it->price && qty <= it->remaining) {
            it->remaining = qty;  // keeps its place in the queue
            return {};
        }
        const Side side = it->side;
        resting_.erase(it);
        return add(id, side, OrderType::Limit, price, qty);
    }

    [[nodiscard]] const std::vector<RefOrder>& resting() const {
        return resting_;
    }

    [[nodiscard]] std::optional<Price> best(Side side) const {
        std::optional<Price> result;
        for (const auto& o : resting_) {
            if (o.side != side)
                continue;
            if (!result || (side == Side::Buy ? o.price > *result : o.price < *result))
                result = o.price;
        }
        return result;
    }

private:
    static bool crosses(Side incoming, Price limit, Price resting_price) {
        return incoming == Side::Buy ? limit >= resting_price : limit <= resting_price;
    }

    // True if `a` should be filled before `b` (both on the same side)
    static bool better(const RefOrder& a, const RefOrder& b) {
        if (a.price != b.price)
            return a.side == Side::Sell ? a.price < b.price : a.price > b.price;
        return a.sequence < b.sequence;
    }

    std::vector<RefOrder>::iterator find(OrderID id) {
        return std::find_if(resting_.begin(), resting_.end(),
                            [id](const RefOrder& o) { return o.id == id; });
    }

    std::vector<RefOrder> resting_;
    uint64_t next_sequence_ = 0;
};

Order make_order(OrderID id, Side side, OrderType type, Price price, Quantity qty) {
    Order o{};
    o.id = id;
    o.side = side;
    o.type = type;
    o.price = price;
    o.initial_quantity = qty;
    o.remaining_quantity = qty;
    return o;
}

void expect_same_trades(const std::vector<Trade>& actual, const std::vector<RefTrade>& expected) {
    ASSERT_EQ(actual.size(), expected.size());
    for (size_t i = 0; i < actual.size(); ++i) {
        EXPECT_EQ(actual[i].buy_order_id, expected[i].buy_id) << "trade " << i;
        EXPECT_EQ(actual[i].sell_order_id, expected[i].sell_id) << "trade " << i;
        EXPECT_EQ(actual[i].price, expected[i].price) << "trade " << i;
        EXPECT_EQ(actual[i].quantity, expected[i].quantity) << "trade " << i;
        EXPECT_NE(actual[i].buy_order_id, actual[i].sell_order_id) << "trade " << i;
        EXPECT_GT(actual[i].quantity, 0u) << "trade " << i;
    }
}

void expect_same_book(const OrderBook& book, const ReferenceBook& reference) {
    std::string why;
    ASSERT_TRUE(book.check_invariants(&why)) << why;
    ASSERT_EQ(book.total_orders(), reference.resting().size());
    EXPECT_EQ(book.best_bid(), reference.best(Side::Buy));
    EXPECT_EQ(book.best_ask(), reference.best(Side::Sell));
    // Plain comparisons here, with one assertion on a mismatch: this loop runs for every
    // resting order after every operation.
    for (const auto& expected : reference.resting()) {
        const Order* actual = book.find_order(expected.id);
        if (actual == nullptr) {
            FAIL() << "order " << expected.id << " is missing from the book";
        }
        if (actual->side != expected.side || actual->price != expected.price ||
            actual->remaining_quantity != expected.remaining) {
            FAIL() << "order " << expected.id << " differs: book has "
                   << side_to_string(actual->side) << " " << actual->remaining_quantity << " @ "
                   << actual->price << ", reference has " << side_to_string(expected.side) << " "
                   << expected.remaining << " @ " << expected.price;
        }
    }
}

struct RunTotals {
    uint64_t trades = 0;
    uint64_t cancels_hit = 0;
    uint64_t rejects = 0;
    size_t deepest_book = 0;
};

// Applies `operations` random requests to both books, comparing after each one.
RunTotals run_differential(uint32_t seed, int operations) {
    std::mt19937 rng(seed);
    auto pick = [&rng](int lo, int hi) { return std::uniform_int_distribution<int>(lo, hi)(rng); };

    OrderBook book("FUZZ");
    ReferenceBook reference;
    RunTotals totals;
    OrderID next_id = 1;

    // A live ID if there is one, otherwise an ID that was never used
    auto live_or_unused_id = [&]() -> OrderID {
        const auto& resting = reference.resting();
        if (resting.empty())
            return next_id + 1000;
        return resting[static_cast<size_t>(pick(0, static_cast<int>(resting.size()) - 1))].id;
    };

    for (int op = 0; op < operations; ++op) {
        SCOPED_TRACE("seed " + std::to_string(seed) + ", operation " + std::to_string(op));
        const int roll = pick(0, 99);

        if (roll < 70) {
            // New order. Limit prices lean passive (bids low, asks high, with a small
            // overlap) so queues build up; the other types use the whole band.
            OrderID id = next_id++;
            const int id_roll = pick(0, 99);
            if (id_roll < 3)
                id = live_or_unused_id();  // usually a duplicate of a resting order
            else if (id_roll == 3)
                id = INVALID_ID;

            const Side side = pick(0, 1) ? Side::Buy : Side::Sell;
            const int type_roll = pick(0, 99);
            const OrderType type = type_roll < 64   ? OrderType::Limit
                                   : type_roll < 76 ? OrderType::Market
                                   : type_roll < 88 ? OrderType::ImmediateOrCancel
                                                    : OrderType::FillOrKill;
            Price price = static_cast<Price>(pick(9990, 10010));
            if (type == OrderType::Limit)
                price =
                    static_cast<Price>(side == Side::Buy ? pick(9990, 10002) : pick(9998, 10010));
            if (pick(0, 99) == 0)
                price = static_cast<Price>(pick(-2, 0));
            Quantity qty = static_cast<Quantity>(pick(1, 60));
            if (pick(0, 99) == 0)
                qty = 0;

            const bool was_resting = book.contains(id);
            const auto expected = reference.add(id, side, type, price, qty);
            const auto actual = book.add_order(make_order(id, side, type, price, qty));
            expect_same_trades(actual, expected);
            totals.trades += actual.size();
            if (book.last_reject() != RejectReason::None)
                ++totals.rejects;
            if (type != OrderType::Limit) {
                // Only a limit order may rest. If the ID is on the book afterwards, it is
                // the order that was already there under that ID.
                EXPECT_EQ(book.contains(id), was_resting);
            }
        } else if (roll < 88) {
            // Cancel: mostly a resting order, sometimes one that is gone or never existed
            const OrderID id = pick(0, 3) == 0
                                   ? static_cast<OrderID>(pick(1, static_cast<int>(next_id) + 5))
                                   : live_or_unused_id();
            const bool expected = reference.cancel(id);
            const bool actual = book.cancel_order(id);
            EXPECT_EQ(actual, expected);
            totals.cancels_hit += actual ? 1u : 0u;
            EXPECT_FALSE(book.contains(id));
        } else {
            // Modify: new price near the old band, new quantity up or down, sometimes invalid
            const OrderID id = pick(0, 5) == 0 ? next_id + 7 : live_or_unused_id();
            Price price = static_cast<Price>(pick(9990, 10010));
            const Order* current = book.find_order(id);
            if (current != nullptr && pick(0, 1) == 0)
                price = current->price;  // same price: exercises the keep-priority path
            if (pick(0, 49) == 0)
                price = 0;
            const Quantity qty = static_cast<Quantity>(pick(0, 80));

            const auto expected = reference.modify(id, price, qty);
            const auto actual = book.modify_order(id, price, qty);
            expect_same_trades(actual, expected);
            totals.trades += actual.size();
        }

        expect_same_book(book, reference);
        if (::testing::Test::HasFatalFailure() || ::testing::Test::HasNonfatalFailure())
            return totals;  // the first divergence is the useful one
        totals.deepest_book = std::max(totals.deepest_book, book.total_orders());
    }
    return totals;
}

}  // namespace

class RandomizedBookTest : public ::testing::TestWithParam<uint32_t> {};

TEST_P(RandomizedBookTest, AgreesWithTheReferenceBookAfterEveryOperation) {
    const RunTotals totals = run_differential(GetParam(), 6000);

    // Guard against a workload that stops exercising the book
    EXPECT_GT(totals.trades, 1000u);
    EXPECT_GT(totals.cancels_hit, 300u);
    EXPECT_GT(totals.rejects, 50u);
    EXPECT_GT(totals.deepest_book, 20u);
    RecordProperty("trades", static_cast<int>(totals.trades));
    RecordProperty("deepest_book", static_cast<int>(totals.deepest_book));
}

INSTANTIATE_TEST_SUITE_P(FixedSeeds, RandomizedBookTest,
                         ::testing::Values(1u, 42u, 20261117u, 0xC0FFEEu, 987654321u));

// A market order has nothing to trade with on an empty book and must not rest.
TEST(BookInvariantTest, MarketOrderAgainstAnEmptyBookDoesNothing) {
    OrderBook book("X");
    auto buys = book.add_order(make_order(1, Side::Buy, OrderType::Market, 0, 100));
    auto sells = book.add_order(make_order(2, Side::Sell, OrderType::Market, 0, 100));

    EXPECT_TRUE(buys.empty());
    EXPECT_TRUE(sells.empty());
    EXPECT_EQ(book.total_orders(), 0u);
    EXPECT_EQ(book.best_bid(), std::nullopt);
    EXPECT_EQ(book.best_ask(), std::nullopt);
    EXPECT_TRUE(book.check_invariants());
}

// A fill-or-kill order that cannot be covered leaves every resting order as it was.
TEST(BookInvariantTest, RejectedFillOrKillLeavesRestingQuantitiesUntouched) {
    OrderBook book("X");
    book.add_order(make_order(1, Side::Sell, OrderType::Limit, 10000, 40));
    book.add_order(make_order(2, Side::Sell, OrderType::Limit, 10010, 40));

    auto trades = book.add_order(make_order(3, Side::Buy, OrderType::FillOrKill, 10010, 81));

    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(book.last_reject(), RejectReason::FillOrKillUnfilled);
    ASSERT_NE(book.find_order(1), nullptr);
    ASSERT_NE(book.find_order(2), nullptr);
    EXPECT_EQ(book.find_order(1)->remaining_quantity, 40u);
    EXPECT_EQ(book.find_order(2)->remaining_quantity, 40u);
    EXPECT_FALSE(book.contains(3));
}

// The remainder of an immediate-or-cancel order is dropped, not rested.
TEST(BookInvariantTest, ImmediateOrCancelRemainderDoesNotRest) {
    OrderBook book("X");
    book.add_order(make_order(1, Side::Sell, OrderType::Limit, 10000, 30));

    auto trades =
        book.add_order(make_order(2, Side::Buy, OrderType::ImmediateOrCancel, 10000, 100));

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].quantity, 30u);
    EXPECT_EQ(book.total_orders(), 0u);
    EXPECT_FALSE(book.contains(2));
}

// An order that has been filled completely is gone: cancel and modify both report that.
TEST(BookInvariantTest, CancelAndModifyOfAFilledOrderDoNothing) {
    OrderBook book("X");
    book.add_order(make_order(1, Side::Sell, OrderType::Limit, 10000, 50));
    book.add_order(make_order(2, Side::Buy, OrderType::Limit, 10000, 50));  // fills order 1

    EXPECT_FALSE(book.cancel_order(1));
    EXPECT_TRUE(book.modify_order(1, 10000, 10).empty());
    EXPECT_EQ(book.last_reject(), RejectReason::UnknownOrder);
    EXPECT_EQ(book.total_orders(), 0u);
}

// Cancelling a partially filled order removes exactly what was left of it.
TEST(BookInvariantTest, CancelOfAPartiallyFilledOrderRemovesTheRemainder) {
    OrderBook book("X");
    book.add_order(make_order(1, Side::Sell, OrderType::Limit, 10000, 100));
    book.add_order(make_order(2, Side::Buy, OrderType::Limit, 10000, 30));

    ASSERT_NE(book.find_order(1), nullptr);
    EXPECT_EQ(book.find_order(1)->remaining_quantity, 70u);
    EXPECT_EQ(book.find_order(1)->filled_quantity(), 30u);

    EXPECT_TRUE(book.cancel_order(1));
    EXPECT_EQ(book.ask_levels(), 0u);
    EXPECT_TRUE(book.check_invariants());
}

// Orders have no owner, so two orders from one participant trade with each other.
// This documents the behaviour; it is a limitation, not a feature.
TEST(BookInvariantTest, NoSelfTradePreventionOrdersHaveNoOwner) {
    OrderBook book("X");
    book.add_order(make_order(1, Side::Sell, OrderType::Limit, 10000, 10));
    auto trades = book.add_order(make_order(2, Side::Buy, OrderType::Limit, 10000, 10));

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].sell_order_id, 1u);
    EXPECT_EQ(trades[0].buy_order_id, 2u);
}
