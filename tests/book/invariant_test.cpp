#include <chrono>

#include <gtest/gtest.h>

#include "orderbook/book/order_book.hpp"

using namespace std::chrono_literals;
using namespace orderbook::core;
using orderbook::book::AddResult;
using orderbook::book::InvariantViolation;
using orderbook::book::OrderBook;

// ---------- Test-only access to the book's private storage ----------
// OrderBook declares `friend struct OrderBookTestAccess;`. Only these tests use it, to break
// the book on purpose and prove the checker notices. Production code never touches it.
namespace orderbook::book {
struct OrderBookTestAccess {
    static auto& bids(OrderBook& book) { return book.bids_; }
    static auto& asks(OrderBook& book) { return book.asks_; }
    static auto& orders_by_id(OrderBook& book) { return book.orders_by_id_; }
};
} // namespace orderbook::book

using orderbook::book::OrderBookTestAccess;

namespace {

// Passes when the checker finds nothing wrong; on failure, prints which invariant broke.
void expect_healthy(const OrderBook& book) {
    const auto violation = book.check_invariants();
    EXPECT_FALSE(violation.has_value())
        << "check_invariants() reported violation #" << static_cast<int>(*violation);
}

} // namespace

// ---------- Shared starting book ----------
// Bids at 18742, front to back: #1 (100), #2 (200). Bid #3 (50) alone at 18740.
// Ask #10 (75) alone at 18745. Four orders, three levels, depth at 18742 is 300.
class InvariantBook : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_EQ(book.add_order(OrderId{1}, Side::Buy, Price{18742}, Qty{100}, Timestamp{1ns}),
                  AddResult::Accepted);
        ASSERT_EQ(book.add_order(OrderId{2}, Side::Buy, Price{18742}, Qty{200}, Timestamp{2ns}),
                  AddResult::Accepted);
        ASSERT_EQ(book.add_order(OrderId{3}, Side::Buy, Price{18740}, Qty{50}, Timestamp{3ns}),
                  AddResult::Accepted);
        ASSERT_EQ(book.add_order(OrderId{10}, Side::Sell, Price{18745}, Qty{75}, Timestamp{4ns}),
                  AddResult::Accepted);
    }

    OrderBook book;
};

// ---------- A healthy book stays healthy through every public operation ----------

TEST(Invariants, EmptyBookIsHealthy) {
    const OrderBook book;
    expect_healthy(book);
}

TEST_F(InvariantBook, HealthyAfterAdds) { expect_healthy(book); }

TEST_F(InvariantBook, HealthyAfterCancellingFrontOfQueue) {
    ASSERT_EQ(book.cancel_order(OrderId{1}), orderbook::book::CancelResult::Cancelled);
    expect_healthy(book);
}

TEST_F(InvariantBook, HealthyAfterCancellingLastOrderAtLevel) {
    ASSERT_EQ(book.cancel_order(OrderId{3}), orderbook::book::CancelResult::Cancelled);
    expect_healthy(book);
}

TEST_F(InvariantBook, HealthyAfterModifyKeepingPlace) {
    ASSERT_EQ(book.modify_order(OrderId{1}, Price{18742}, Qty{60}, Timestamp{5ns}),
              orderbook::book::ModifyResult::Modified);
    expect_healthy(book);
}

TEST_F(InvariantBook, HealthyAfterModifyIncreasingQty) {
    ASSERT_EQ(book.modify_order(OrderId{1}, Price{18742}, Qty{150}, Timestamp{5ns}),
              orderbook::book::ModifyResult::Modified);
    expect_healthy(book);
}

TEST_F(InvariantBook, HealthyAfterModifyMovingOnlyOrderToNewPrice) {
    ASSERT_EQ(book.modify_order(OrderId{3}, Price{18741}, Qty{50}, Timestamp{5ns}),
              orderbook::book::ModifyResult::Modified);
    expect_healthy(book);
}

TEST_F(InvariantBook, HealthyAfterReduce) {
    ASSERT_EQ(book.reduce_order(OrderId{2}, Qty{50}), orderbook::book::ReduceResult::Reduced);
    expect_healthy(book);
}

TEST_F(InvariantBook, HealthyAfterReduceRemovingOrder) {
    ASSERT_EQ(book.reduce_order(OrderId{3}, Qty{50}), orderbook::book::ReduceResult::Removed);
    expect_healthy(book);
}

TEST_F(InvariantBook, HealthyAfterReplace) {
    ASSERT_EQ(book.replace_order(OrderId{1}, OrderId{20}, Price{18743}, Qty{100}, Timestamp{5ns}),
              orderbook::book::ReplaceResult::Replaced);
    expect_healthy(book);
}

TEST_F(InvariantBook, HealthyAfterRejectedOperations) {
    ASSERT_EQ(book.add_order(OrderId{1}, Side::Sell, Price{18750}, Qty{10}, Timestamp{5ns}),
              AddResult::DuplicateOrderId);
    ASSERT_EQ(book.cancel_order(OrderId{99}), orderbook::book::CancelResult::UnknownOrderId);
    ASSERT_EQ(book.modify_order(OrderId{1}, Price{18742}, Qty{0}, Timestamp{6ns}),
              orderbook::book::ModifyResult::ZeroQuantity);
    expect_healthy(book);
}

// Queries must never create levels (the `bids_[price]` trap from print_book).
TEST_F(InvariantBook, HealthyAfterQueriesOnMissingPrices) {
    EXPECT_EQ(book.depth_at(Side::Buy, Price{18700}), Qty{0});
    EXPECT_EQ(book.depth_at(Side::Sell, Price{18800}), Qty{0});
    EXPECT_FALSE(book.find_order(OrderId{99}).has_value());
    EXPECT_FALSE(book.queue_ahead(OrderId{99}).has_value());
    expect_healthy(book);
}

// ---------- The checker notices each kind of damage ----------
// Each test breaks the book through OrderBookTestAccess and expects one specific report.
// Precedence the checker follows, when one corruption breaks two things:
//   an order's own checks (qty, side, price, index entry) come before its level's depth check,
//   and the walk over all orders comes before the final count comparison.

TEST_F(InvariantBook, ReportsOrderWithZeroQty) {
    OrderBookTestAccess::orders_by_id(book).at(OrderId{1})->qty = Qty{0};
    EXPECT_EQ(book.check_invariants(), InvariantViolation::NonPositiveQty);
}

TEST_F(InvariantBook, ReportsSellOrderInBids) {
    OrderBookTestAccess::orders_by_id(book).at(OrderId{3})->side = Side::Sell;
    EXPECT_EQ(book.check_invariants(), InvariantViolation::WrongSide);
}

TEST_F(InvariantBook, ReportsBuyOrderInAsks) {
    OrderBookTestAccess::orders_by_id(book).at(OrderId{10})->side = Side::Buy;
    EXPECT_EQ(book.check_invariants(), InvariantViolation::WrongSide);
}

TEST_F(InvariantBook, ReportsOrderWhosePriceDiffersFromItsLevel) {
    OrderBookTestAccess::orders_by_id(book).at(OrderId{2})->price = Price{18741};
    EXPECT_EQ(book.check_invariants(), InvariantViolation::WrongPrice);
}

TEST_F(InvariantBook, ReportsEmptyLevel) {
    OrderBookTestAccess::bids(book)[Price{18700}];  // operator[] creates an empty level
    EXPECT_EQ(book.check_invariants(), InvariantViolation::EmptyLevel);
}

TEST_F(InvariantBook, ReportsEmptyAskLevel) {
    OrderBookTestAccess::asks(book)[Price{18800}];
    EXPECT_EQ(book.check_invariants(), InvariantViolation::EmptyLevel);
}

// Only depth breaks: #3 is alone at 18740; stored depth stays 50, orders now sum to 100.
// Qty, side, price, index and count are all unchanged.
TEST_F(InvariantBook, ReportsDepthThatDisagreesWithOrders) {
    OrderBookTestAccess::orders_by_id(book).at(OrderId{3})->qty = Qty{100};
    EXPECT_EQ(book.check_invariants(), InvariantViolation::DepthMismatch);
}

TEST_F(InvariantBook, ReportsQueuedOrderMissingFromIndex) {
    OrderBookTestAccess::orders_by_id(book).erase(OrderId{3});
    EXPECT_EQ(book.check_invariants(), InvariantViolation::IndexMismatch);
}

TEST_F(InvariantBook, ReportsIndexEntriesPointingAtTheWrongOrders) {
    auto& index = OrderBookTestAccess::orders_by_id(book);
    std::swap(index.at(OrderId{1}), index.at(OrderId{2}));  // both ids exist, both point wrong
    EXPECT_EQ(book.check_invariants(), InvariantViolation::IndexMismatch);
}

// Only the count breaks: #4 is a new index entry pointing at #2's order, with no order of
// its own. Every queued order still finds itself in the walk, so only the final count
// (4 queued vs 5 entries) catches it.
TEST_F(InvariantBook, ReportsIndexEntryWithNoQueuedOrder) {
    auto& index = OrderBookTestAccess::orders_by_id(book);
    index.emplace(OrderId{4}, index.at(OrderId{2}));
    EXPECT_EQ(book.check_invariants(), InvariantViolation::CountMismatch);
}
