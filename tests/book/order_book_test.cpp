#include <chrono>
#include <sstream>

#include <gtest/gtest.h>

#include "orderbook/book/order_book.hpp"

using namespace std::chrono_literals;
using namespace orderbook::core;
using orderbook::book::AddResult;
using orderbook::book::CancelResult;
using orderbook::book::ModifyResult;
using orderbook::book::OrderBook;
using orderbook::book::ReduceResult;
using orderbook::book::ReplaceResult;

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
    const AddResult result =
        book.add_order(OrderId{1}, Side::Buy, Price{12346}, Qty{102}, Timestamp{2ns});
    EXPECT_EQ(result, AddResult::DuplicateOrderId);

    // The best bid must be unchanged.
    EXPECT_EQ(book.best_bid(), Price{12345});
}

TEST(OrderBook, RejectsZeroQuantity) {
    OrderBook book;
    const AddResult result =
        book.add_order(OrderId{1}, Side::Buy, Price{12345}, Qty{0}, Timestamp{1ns});
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

TEST(OrderBook, CancelLowersDepthAndKeepsLevelWithOtherOrders) {
    OrderBook book;
    ASSERT_EQ(book.add_order(OrderId{1}, Side::Buy, Price{18742}, Qty{100}, Timestamp{1ns}),
              AddResult::Accepted);
    ASSERT_EQ(book.add_order(OrderId{2}, Side::Buy, Price{18742}, Qty{200}, Timestamp{2ns}),
              AddResult::Accepted);

    EXPECT_EQ(book.cancel_order(OrderId{1}), CancelResult::Cancelled);
    EXPECT_EQ(book.depth_at(Side::Buy, Price{18742}), Qty{200});
    EXPECT_EQ(book.best_bid(), Price{18742});
}

TEST(OrderBook, CancellingLastBidAtBestPriceMakesNextPriceBest) {
    OrderBook book;
    ASSERT_EQ(book.add_order(OrderId{1}, Side::Buy, Price{18742}, Qty{100}, Timestamp{1ns}),
              AddResult::Accepted);
    ASSERT_EQ(book.add_order(OrderId{2}, Side::Buy, Price{18740}, Qty{50}, Timestamp{2ns}),
              AddResult::Accepted);

    EXPECT_EQ(book.cancel_order(OrderId{1}), CancelResult::Cancelled);
    EXPECT_EQ(book.best_bid(), Price{18740});
}

TEST(OrderBook, CancellingLastAskAtBestPriceMakesNextPriceBest) {
    OrderBook book;
    ASSERT_EQ(book.add_order(OrderId{1}, Side::Sell, Price{18745}, Qty{75}, Timestamp{1ns}),
              AddResult::Accepted);
    ASSERT_EQ(book.add_order(OrderId{2}, Side::Sell, Price{18750}, Qty{120}, Timestamp{2ns}),
              AddResult::Accepted);

    EXPECT_EQ(book.cancel_order(OrderId{1}), CancelResult::Cancelled);
    EXPECT_EQ(book.best_ask(), Price{18750});
}

TEST(OrderBook, CancellingOnlyOrderLeavesEmptyBook) {
    OrderBook book;
    ASSERT_EQ(book.add_order(OrderId{1}, Side::Buy, Price{18742}, Qty{100}, Timestamp{1ns}),
              AddResult::Accepted);

    EXPECT_EQ(book.cancel_order(OrderId{1}), CancelResult::Cancelled);
    EXPECT_FALSE(book.best_bid().has_value());

    // A leftover empty level would still print a row.
    std::ostringstream out;
    book.print_book(out);
    EXPECT_EQ(out.str(), "");
}

TEST(OrderBook, CancellingUnknownIdLeavesBookUnchanged) {
    OrderBook book;
    ASSERT_EQ(book.add_order(OrderId{1}, Side::Buy, Price{18742}, Qty{100}, Timestamp{1ns}),
              AddResult::Accepted);

    EXPECT_EQ(book.cancel_order(OrderId{99}), CancelResult::UnknownOrderId);
    EXPECT_EQ(book.depth_at(Side::Buy, Price{18742}), Qty{100});
    EXPECT_EQ(book.best_bid(), Price{18742});
}

TEST(OrderBook, CancellingSameIdTwiceIsUnknownTheSecondTime) {
    OrderBook book;
    ASSERT_EQ(book.add_order(OrderId{1}, Side::Buy, Price{18742}, Qty{100}, Timestamp{1ns}),
              AddResult::Accepted);

    ASSERT_EQ(book.cancel_order(OrderId{1}), CancelResult::Cancelled);
    EXPECT_EQ(book.cancel_order(OrderId{1}), CancelResult::UnknownOrderId);
}

TEST(OrderBook, CancelledIdCanBeAddedAgain) {
    OrderBook book;
    ASSERT_EQ(book.add_order(OrderId{1}, Side::Buy, Price{18742}, Qty{100}, Timestamp{1ns}),
              AddResult::Accepted);
    ASSERT_EQ(book.cancel_order(OrderId{1}), CancelResult::Cancelled);

    EXPECT_EQ(book.add_order(OrderId{1}, Side::Sell, Price{18745}, Qty{75}, Timestamp{2ns}),
              AddResult::Accepted);
    EXPECT_EQ(book.best_ask(), Price{18745});
}

// ---------- Shared starting book for queries, modify, reduce and replace ----------
// Queue at 18742, front to back: #1 (100), #2 (200), #3 (50). Then #4 (50) alone at 18740.
// Depth at 18742 is 350.
class BookWithQueue : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_EQ(book.add_order(OrderId{1}, Side::Buy, Price{18742}, Qty{100}, Timestamp{1ns}),
                  AddResult::Accepted);
        ASSERT_EQ(book.add_order(OrderId{2}, Side::Buy, Price{18742}, Qty{200}, Timestamp{2ns}),
                  AddResult::Accepted);
        ASSERT_EQ(book.add_order(OrderId{3}, Side::Buy, Price{18742}, Qty{50}, Timestamp{3ns}),
                  AddResult::Accepted);
        ASSERT_EQ(book.add_order(OrderId{4}, Side::Buy, Price{18740}, Qty{50}, Timestamp{4ns}),
                  AddResult::Accepted);
    }

    OrderBook book;
};

// ---------- Queries ----------

TEST_F(BookWithQueue, FindOrderReturnsRestingOrder) {
    const auto order = book.find_order(OrderId{2});
    ASSERT_TRUE(order.has_value());
    EXPECT_EQ(order->id, OrderId{2});
    EXPECT_EQ(order->side, Side::Buy);
    EXPECT_EQ(order->price, Price{18742});
    EXPECT_EQ(order->qty, Qty{200});
    EXPECT_EQ(order->arrival_time, Timestamp{2ns});
}

TEST(OrderBook, FindOrderOfUnknownIdIsEmpty) {
    const OrderBook book;
    EXPECT_FALSE(book.find_order(OrderId{1}).has_value());
}

TEST_F(BookWithQueue, QueueAheadCountsEarlierOrdersAtSamePriceOnly) {
    EXPECT_EQ(book.queue_ahead(OrderId{1}), Qty{0});
    EXPECT_EQ(book.queue_ahead(OrderId{2}), Qty{100});
    EXPECT_EQ(book.queue_ahead(OrderId{3}), Qty{300});
    EXPECT_EQ(book.queue_ahead(OrderId{4}), Qty{0});  // other price: nothing ahead
}

TEST(OrderBook, QueueAheadOfUnknownIdIsEmpty) {
    const OrderBook book;
    EXPECT_FALSE(book.queue_ahead(OrderId{1}).has_value());
}

// ---------- Modify (CME priority rules) ----------
// A reduction keeps its place; an increase or a new price goes to the back of the queue.

TEST_F(BookWithQueue, ModifyReducingQtyKeepsQueuePlace) {
    EXPECT_EQ(book.modify_order(OrderId{1}, Price{18742}, Qty{60}, Timestamp{9ns}),
              ModifyResult::Modified);
    EXPECT_EQ(book.queue_ahead(OrderId{1}), Qty{0});
    EXPECT_EQ(book.queue_ahead(OrderId{2}), Qty{60});
    EXPECT_EQ(book.depth_at(Side::Buy, Price{18742}), Qty{310});
    EXPECT_EQ(book.find_order(OrderId{1}).value().arrival_time, Timestamp{1ns});  // kept its time
}

TEST_F(BookWithQueue, ModifyIncreasingQtyMovesToBackOfQueue) {
    EXPECT_EQ(book.modify_order(OrderId{1}, Price{18742}, Qty{150}, Timestamp{9ns}),
              ModifyResult::Modified);
    EXPECT_EQ(book.queue_ahead(OrderId{1}), Qty{250});  // behind #2 (200) and #3 (50)
    EXPECT_EQ(book.queue_ahead(OrderId{2}), Qty{0});
    EXPECT_EQ(book.depth_at(Side::Buy, Price{18742}), Qty{400});
    EXPECT_EQ(book.find_order(OrderId{1}).value().arrival_time, Timestamp{9ns});  // new time
}

TEST_F(BookWithQueue, ModifyChangingPriceMovesToBackOfNewLevel) {
    EXPECT_EQ(book.modify_order(OrderId{1}, Price{18740}, Qty{100}, Timestamp{9ns}),
              ModifyResult::Modified);
    EXPECT_EQ(book.depth_at(Side::Buy, Price{18742}), Qty{250});
    EXPECT_EQ(book.depth_at(Side::Buy, Price{18740}), Qty{150});
    EXPECT_EQ(book.queue_ahead(OrderId{1}), Qty{50});  // behind #4
    EXPECT_EQ(book.find_order(OrderId{1}).value().price, Price{18740});
}

TEST_F(BookWithQueue, ModifyWithSamePriceAndQtyKeepsQueuePlace) {
    EXPECT_EQ(book.modify_order(OrderId{1}, Price{18742}, Qty{100}, Timestamp{9ns}),
              ModifyResult::Modified);
    EXPECT_EQ(book.queue_ahead(OrderId{2}), Qty{100});
    EXPECT_EQ(book.find_order(OrderId{1}).value().arrival_time, Timestamp{1ns});
}

TEST_F(BookWithQueue, ModifyMovingOnlyOrderAwayRemovesItsOldLevel) {
    EXPECT_EQ(book.modify_order(OrderId{4}, Price{18744}, Qty{50}, Timestamp{9ns}),
              ModifyResult::Modified);
    EXPECT_EQ(book.best_bid(), Price{18744});

    // A leftover empty level at 18740 would still print a row.
    std::ostringstream out;
    book.print_book(out);
    EXPECT_EQ(out.str(),
              "BID 18744 qty=50 orders=1\n"
              "BID 18742 qty=350 orders=3\n");
}

TEST_F(BookWithQueue, ModifyUnknownIdIsRejected) {
    EXPECT_EQ(book.modify_order(OrderId{99}, Price{18742}, Qty{10}, Timestamp{9ns}),
              ModifyResult::UnknownOrderId);
    EXPECT_EQ(book.depth_at(Side::Buy, Price{18742}), Qty{350});
}

TEST_F(BookWithQueue, ModifyToZeroQtyIsRejectedAndLeavesOrderUnchanged) {
    EXPECT_EQ(book.modify_order(OrderId{1}, Price{18742}, Qty{0}, Timestamp{9ns}),
              ModifyResult::ZeroQuantity);
    EXPECT_EQ(book.find_order(OrderId{1}).value().qty, Qty{100});
    EXPECT_EQ(book.queue_ahead(OrderId{2}), Qty{100});
    EXPECT_EQ(book.depth_at(Side::Buy, Price{18742}), Qty{350});
}

// ---------- Reduce (cancel part of an order by an amount; always keeps place) ----------

TEST_F(BookWithQueue, ReduceLowersQtyAndKeepsQueuePlace) {
    EXPECT_EQ(book.reduce_order(OrderId{1}, Qty{30}), ReduceResult::Reduced);
    EXPECT_EQ(book.find_order(OrderId{1}).value().qty, Qty{70});
    EXPECT_EQ(book.queue_ahead(OrderId{2}), Qty{70});
    EXPECT_EQ(book.depth_at(Side::Buy, Price{18742}), Qty{320});
}

TEST_F(BookWithQueue, ReduceByWholeRemainingRemovesOrder) {
    EXPECT_EQ(book.reduce_order(OrderId{2}, Qty{200}), ReduceResult::Removed);
    EXPECT_FALSE(book.find_order(OrderId{2}).has_value());
    EXPECT_EQ(book.queue_ahead(OrderId{3}), Qty{100});
    EXPECT_EQ(book.depth_at(Side::Buy, Price{18742}), Qty{150});
}

TEST_F(BookWithQueue, ReduceByMoreThanRemainingRemovesOrderAndItsLevel) {
    EXPECT_EQ(book.reduce_order(OrderId{4}, Qty{500}), ReduceResult::Removed);
    EXPECT_FALSE(book.find_order(OrderId{4}).has_value());

    std::ostringstream out;
    book.print_book(out);
    EXPECT_EQ(out.str(), "BID 18742 qty=350 orders=3\n");
}

TEST_F(BookWithQueue, ReduceByZeroIsRejected) {
    EXPECT_EQ(book.reduce_order(OrderId{1}, Qty{0}), ReduceResult::ZeroQuantity);
    EXPECT_EQ(book.depth_at(Side::Buy, Price{18742}), Qty{350});
}

TEST_F(BookWithQueue, ReduceUnknownIdIsRejected) {
    EXPECT_EQ(book.reduce_order(OrderId{99}, Qty{10}), ReduceResult::UnknownOrderId);
    EXPECT_EQ(book.depth_at(Side::Buy, Price{18742}), Qty{350});
}

// ---------- Replace (retire an id, add a new one at the back; ITCH "U") ----------

TEST_F(BookWithQueue, ReplaceRetiresOldIdAndAddsNewIdAtBack) {
    EXPECT_EQ(book.replace_order(OrderId{1}, OrderId{7}, Price{18742}, Qty{100}, Timestamp{9ns}),
              ReplaceResult::Replaced);
    EXPECT_FALSE(book.find_order(OrderId{1}).has_value());
    EXPECT_EQ(book.find_order(OrderId{7}).value().qty, Qty{100});
    // The side comes from the old order.
    EXPECT_EQ(book.find_order(OrderId{7}).value().side, Side::Buy);
    EXPECT_EQ(book.queue_ahead(OrderId{7}), Qty{250});
    EXPECT_EQ(book.queue_ahead(OrderId{2}), Qty{0});
}

TEST_F(BookWithQueue, ReplaceToExistingIdIsRejectedAndKeepsOldOrder) {
    // Checks must run before anything changes, or the old order would already be gone.
    EXPECT_EQ(book.replace_order(OrderId{1}, OrderId{2}, Price{18742}, Qty{100}, Timestamp{9ns}),
              ReplaceResult::DuplicateOrderId);
    EXPECT_EQ(book.find_order(OrderId{1}).value().qty, Qty{100});
    EXPECT_EQ(book.queue_ahead(OrderId{2}), Qty{100});
    EXPECT_EQ(book.depth_at(Side::Buy, Price{18742}), Qty{350});
}

TEST_F(BookWithQueue, ReplaceKeepingSameIdMovesToBack) {
    EXPECT_EQ(book.replace_order(OrderId{1}, OrderId{1}, Price{18742}, Qty{100}, Timestamp{9ns}),
              ReplaceResult::Replaced);
    EXPECT_EQ(book.queue_ahead(OrderId{1}), Qty{250});
}

TEST_F(BookWithQueue, ReplaceUnknownIdIsRejected) {
    EXPECT_EQ(book.replace_order(OrderId{99}, OrderId{7}, Price{18742}, Qty{10}, Timestamp{9ns}),
              ReplaceResult::UnknownOrderId);
    EXPECT_FALSE(book.find_order(OrderId{7}).has_value());
}

TEST_F(BookWithQueue, ReplaceToZeroQtyIsRejectedAndKeepsOldOrder) {
    EXPECT_EQ(book.replace_order(OrderId{1}, OrderId{7}, Price{18742}, Qty{0}, Timestamp{9ns}),
              ReplaceResult::ZeroQuantity);
    EXPECT_EQ(book.find_order(OrderId{1}).value().qty, Qty{100});
    EXPECT_FALSE(book.find_order(OrderId{7}).has_value());
}
