#include <chrono>

#include <gtest/gtest.h>

#include "orderbook/book/order_book.hpp"

using namespace std::chrono_literals;
using namespace orderbook::core;
using orderbook::book::AddResult;
using orderbook::book::OrderBook;

TEST(OrderBook, EmptyBookHasNoBestPrices) {
    const OrderBook book;
    EXPECT_FALSE(book.best_bid().has_value());
    EXPECT_FALSE(book.best_ask().has_value());
}

TEST(OrderBook, ReturnsBestBidAndAsk) {
    OrderBook book;
    ASSERT_EQ(
        book.add_order(OrderId{1}, Side::Buy, Price{12345}, Qty{100}, Timestamp{1ns}),
        AddResult::Accepted
    );
    ASSERT_EQ(
        book.add_order(OrderId{2}, Side::Sell, Price{12350}, Qty{99}, Timestamp{2ns}),
        AddResult::Accepted
    );
    EXPECT_EQ(book.best_bid(), Price{12345});
    EXPECT_EQ(book.best_ask(), Price{12350});
}

TEST(OrderBook, HigherBidBecomesBest) {
    OrderBook book;
    ASSERT_EQ(
        book.add_order(OrderId{1}, Side::Buy, Price{12345}, Qty{100}, Timestamp{1ns}),
        AddResult::Accepted
    );
    ASSERT_EQ(
        book.add_order(OrderId{2}, Side::Buy, Price{12346}, Qty{98}, Timestamp{2ns}),
        AddResult::Accepted
    );
    EXPECT_EQ(book.best_bid(), Price{12346});
}

TEST(OrderBook, LowerAskBecomesBest) {
    OrderBook book;
    ASSERT_EQ(
        book.add_order(OrderId{1}, Side::Sell, Price{12350}, Qty{99}, Timestamp{1ns}),
        AddResult::Accepted
    );
    ASSERT_EQ(
        book.add_order(OrderId{2}, Side::Sell, Price{12349}, Qty{101}, Timestamp{2ns}),
        AddResult::Accepted
    );
    EXPECT_EQ(book.best_ask(), Price{12349});
}

TEST(OrderBook, RejectsDuplicateOrderId) {
    OrderBook book;
    ASSERT_EQ(
        book.add_order(OrderId{1}, Side::Buy, Price{12345}, Qty{100}, Timestamp{1ns}),
        AddResult::Accepted
    );
    const AddResult result = book.add_order(OrderId{1}, Side::Buy, Price{12346}, Qty{102}, Timestamp{2ns});
    EXPECT_EQ(result, AddResult::DuplicateOrderId);

    // The best bid must be unchanged.
    EXPECT_EQ(book.best_bid(), Price{12345});
}

TEST(OrderBook, RejectsZeroQuantity) {
    OrderBook book;
    const AddResult result = book.add_order(OrderId{1}, Side::Buy, Price{12345}, Qty{0}, Timestamp{1ns});
    EXPECT_EQ(result, AddResult::ZeroQuantity);

    // The best bid must not have a value.
    EXPECT_FALSE(book.best_bid().has_value());
}

TEST(OrderBook, DepthIsTotalAtPriceLevel) {
    OrderBook book;
    ASSERT_EQ(book.add_order(OrderId{1}, Side::Buy, Price{12345}, Qty{100}, Timestamp{1ns}),
              AddResult::Accepted);
    ASSERT_EQ(book.add_order(OrderId{3}, Side::Buy, Price{12345}, Qty{200}, Timestamp{2ns}),
              AddResult::Accepted);
    EXPECT_EQ(book.depth_at(Side::Buy, Price{12345}), Qty{300});
}

TEST(OrderBook, DepthIsZeroWhereNoOrdersRest) {
    const OrderBook book;
    EXPECT_EQ(book.depth_at(Side::Buy, Price{12345}), Qty{0});
}

TEST(OrderBook, DepthOfOtherSideIsZero) {
    OrderBook book;
    ASSERT_EQ(book.add_order(OrderId{1}, Side::Buy, Price{12345}, Qty{100}, Timestamp{1ns}),
              AddResult::Accepted);
    EXPECT_EQ(book.depth_at(Side::Sell, Price{12345}), Qty{0});
}

TEST(OrderBook, RejectedDuplicateLeavesDepthUnchanged) {
    OrderBook book;
    ASSERT_EQ(book.add_order(OrderId{1}, Side::Buy, Price{12345}, Qty{100}, Timestamp{1ns}),
              AddResult::Accepted);
    const AddResult result =
        book.add_order(OrderId{1}, Side::Buy, Price{12345}, Qty{50}, Timestamp{2ns});
    EXPECT_EQ(result, AddResult::DuplicateOrderId);
    EXPECT_EQ(book.depth_at(Side::Buy, Price{12345}), Qty{100});
}
