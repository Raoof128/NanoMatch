#include "nanomatch/engine/order_validator.hpp"

#include <gtest/gtest.h>

using namespace nm;

// Helper to build an OrderRequest easily
static OrderRequest make_request(OrderRequest::Action action, OrderID id, Side side, OrderType type,
                                 Price price, Quantity qty) {
    OrderRequest req{};
    req.action = action;
    req.id = id;
    req.side = side;
    req.type = type;
    req.price = price;
    req.quantity = qty;
    return req;
}

TEST(OrderValidatorTest, ValidLimitOrderPasses) {
    auto req = make_request(OrderRequest::Action::New, 1, Side::Buy, OrderType::Limit, 10000, 100);
    auto result = OrderValidator::validate(req);
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.error.empty());
}

TEST(OrderValidatorTest, ZeroQuantityRejected) {
    auto req = make_request(OrderRequest::Action::New, 1, Side::Buy, OrderType::Limit, 10000, 0);
    auto result = OrderValidator::validate(req);
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.error.empty());
}

TEST(OrderValidatorTest, NegativePriceRejectedForLimitOrder) {
    auto req = make_request(OrderRequest::Action::New, 1, Side::Buy, OrderType::Limit, -100, 100);
    auto result = OrderValidator::validate(req);
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.error.empty());
}

TEST(OrderValidatorTest, ZeroPriceRejectedForLimitOrder) {
    auto req = make_request(OrderRequest::Action::New, 1, Side::Sell, OrderType::Limit, 0, 100);
    auto result = OrderValidator::validate(req);
    EXPECT_FALSE(result.valid);
}

TEST(OrderValidatorTest, CancelWithInvalidIDRejected) {
    auto req =
        make_request(OrderRequest::Action::Cancel, INVALID_ID, Side::Buy, OrderType::Limit, 0, 0);
    auto result = OrderValidator::validate(req);
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.error.empty());
}

TEST(OrderValidatorTest, ModifyWithInvalidIDRejected) {
    auto req = make_request(OrderRequest::Action::Modify, INVALID_ID, Side::Buy, OrderType::Limit,
                            10000, 100);
    auto result = OrderValidator::validate(req);
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.error.empty());
}

TEST(OrderValidatorTest, ModifyWithZeroQuantityRejected) {
    auto req = make_request(OrderRequest::Action::Modify, 5, Side::Buy, OrderType::Limit, 10000, 0);
    auto result = OrderValidator::validate(req);
    EXPECT_FALSE(result.valid);
}

TEST(OrderValidatorTest, ModifyWithZeroPriceRejected) {
    auto req = make_request(OrderRequest::Action::Modify, 5, Side::Buy, OrderType::Limit, 0, 100);
    auto result = OrderValidator::validate(req);
    EXPECT_FALSE(result.valid);
}

TEST(OrderValidatorTest, ModifyWithNegativePriceRejected) {
    auto req =
        make_request(OrderRequest::Action::Modify, 5, Side::Sell, OrderType::Limit, -500, 100);
    auto result = OrderValidator::validate(req);
    EXPECT_FALSE(result.valid);
}

TEST(OrderValidatorTest, MarketOrderWithZeroPriceAccepted) {
    auto req = make_request(OrderRequest::Action::New, 1, Side::Buy, OrderType::Market, 0, 100);
    auto result = OrderValidator::validate(req);
    EXPECT_TRUE(result.valid);
}

TEST(OrderValidatorTest, ValidCancelPasses) {
    auto req = make_request(OrderRequest::Action::Cancel, 42, Side::Buy, OrderType::Limit, 0, 0);
    auto result = OrderValidator::validate(req);
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.error.empty());
}

TEST(OrderValidatorTest, ValidModifyPasses) {
    auto req =
        make_request(OrderRequest::Action::Modify, 10, Side::Sell, OrderType::Limit, 9500, 200);
    auto result = OrderValidator::validate(req);
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.error.empty());
}

TEST(OrderValidatorTest, NewSellLimitOrderValid) {
    auto req = make_request(OrderRequest::Action::New, 1, Side::Sell, OrderType::Limit, 15000, 500);
    auto result = OrderValidator::validate(req);
    EXPECT_TRUE(result.valid);
}

TEST(OrderValidatorTest, IOCOrderWithValidFieldsPasses) {
    auto req = make_request(OrderRequest::Action::New, 1, Side::Buy, OrderType::ImmediateOrCancel,
                            10000, 100);
    auto result = OrderValidator::validate(req);
    EXPECT_TRUE(result.valid);
}

TEST(OrderValidatorTest, FOKOrderWithZeroQuantityFails) {
    auto req =
        make_request(OrderRequest::Action::New, 1, Side::Buy, OrderType::FillOrKill, 10000, 0);
    auto result = OrderValidator::validate(req);
    EXPECT_FALSE(result.valid);
}
