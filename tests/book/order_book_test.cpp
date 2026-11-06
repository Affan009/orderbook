#include <chrono>
#include <sstream>

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

TEST(OrderBook, PrintsNothingForEmptyBook) {
    const OrderBook book;
    std::ostringstream out;
    book.print_book(out);
    EXPECT_EQ(out.str(), "");
}

TEST(OrderBook, PrintsLadderWithBothSides) {
    OrderBook book;
    ASSERT_EQ(book.add_order(OrderId{1}, Side::Buy, Price{18742}, Qty{100}, Timestamp{1ns}),
              AddResult::Accepted);
    ASSERT_EQ(book.add_order(OrderId{2}, Side::Buy, Price{18742}, Qty{200}, Timestamp{2ns}),
              AddResult::Accepted);
    ASSERT_EQ(book.add_order(OrderId{3}, Side::Buy, Price{18740}, Qty{50}, Timestamp{3ns}),
              AddResult::Accepted);
    ASSERT_EQ(book.add_order(OrderId{4}, Side::Sell, Price{18745}, Qty{75}, Timestamp{4ns}),
              AddResult::Accepted);
    ASSERT_EQ(book.add_order(OrderId{5}, Side::Sell, Price{18750}, Qty{120}, Timestamp{5ns}),
              AddResult::Accepted);
    ASSERT_EQ(book.add_order(OrderId{6}, Side::Sell, Price{18750}, Qty{30}, Timestamp{6ns}),
              AddResult::Accepted);

    std::ostringstream out;
    book.print_book(out);
    EXPECT_EQ(out.str(),
              "ASK 18750 qty=150 orders=2\n"
              "ASK 18745 qty=75 orders=1\n"
              "--- spread 3 ---\n"
              "BID 18742 qty=300 orders=2\n"
              "BID 18740 qty=50 orders=1\n");
}

TEST(OrderBook, PrintsOnlyBidsWhenNoAsks) {
    OrderBook book;
    // Worst price arrives first, so arrival order can't produce the expected output.
    ASSERT_EQ(book.add_order(OrderId{1}, Side::Buy, Price{18740}, Qty{50}, Timestamp{1ns}),
              AddResult::Accepted);
    ASSERT_EQ(book.add_order(OrderId{2}, Side::Buy, Price{18742}, Qty{100}, Timestamp{2ns}),
              AddResult::Accepted);

    std::ostringstream out;
    book.print_book(out);
    EXPECT_EQ(out.str(),
              "BID 18742 qty=100 orders=1\n"
              "BID 18740 qty=50 orders=1\n");
}

TEST(OrderBook, PrintsOnlyAsksWhenNoBids) {
    OrderBook book;
    // Best price arrives first, so arrival order can't produce the expected output.
    ASSERT_EQ(book.add_order(OrderId{1}, Side::Sell, Price{18745}, Qty{75}, Timestamp{1ns}),
              AddResult::Accepted);
    ASSERT_EQ(book.add_order(OrderId{2}, Side::Sell, Price{18750}, Qty{150}, Timestamp{2ns}),
              AddResult::Accepted);

    std::ostringstream out;
    book.print_book(out);
    EXPECT_EQ(out.str(),
              "ASK 18750 qty=150 orders=1\n"
              "ASK 18745 qty=75 orders=1\n");
}
