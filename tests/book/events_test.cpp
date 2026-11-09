#include <chrono>
#include <cstdint>
#include <map>
#include <ostream>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

#include "orderbook/book/order_book.hpp"

using namespace std::chrono_literals;
using namespace orderbook::core;
using orderbook::book::AddResult;
using orderbook::book::BookEvent;
using orderbook::book::CancelResult;
using orderbook::book::ModifyResult;
using orderbook::book::Operation;
using orderbook::book::OrderAdded;
using orderbook::book::OrderBook;
using orderbook::book::OrderBookListener;
using orderbook::book::OrderReduced;
using orderbook::book::OrderRejected;
using orderbook::book::OrderRemoved;
using orderbook::book::ReduceResult;
using orderbook::book::RemoveReason;
using orderbook::book::ReplaceResult;

// ---------- Readable failure messages ----------
// GoogleTest prints unknown structs as raw bytes. These overloads make a failing comparison show
// the fields.
// They must live in the events' namespace so GoogleTest finds them.
namespace orderbook::book {
void PrintTo(const OrderAdded& e, std::ostream* os) {
    *os << "Added{seq=" << e.seq << " id=" << e.id.value() << " side=" << static_cast<int>(e.side)
        << " price=" << e.price.value() << " qty=" << e.qty.value() << " last=" << e.last << "}";
}
void PrintTo(const OrderReduced& e, std::ostream* os) {
    *os << "Reduced{seq=" << e.seq << " id=" << e.id.value() << " side=" << static_cast<int>(e.side)
        << " price=" << e.price.value() << " qty=" << e.qty.value() << " by=" << e.by.value()
        << " last=" << e.last << "}";
}
void PrintTo(const OrderRemoved& e, std::ostream* os) {
    *os << "Removed{seq=" << e.seq << " id=" << e.id.value() << " side=" << static_cast<int>(e.side)
        << " price=" << e.price.value() << " qty=" << e.qty.value()
        << " reason=" << static_cast<int>(e.reason) << " last=" << e.last << "}";
}
void PrintTo(const OrderRejected& e, std::ostream* os) {
    *os << "Rejected{seq=" << e.seq << " operation=" << static_cast<int>(e.operation)
        << " id=" << e.id.value() << " reason=(alternative " << e.reason.index() << ", value ";
    std::visit([os](auto value) { *os << static_cast<int>(value); }, e.reason);
    *os << ") last=" << e.last << "}";
}
} // namespace orderbook::book

namespace {


// Writes down every event it receives, in order, as BookEvents (one value holding any of the four).
class RecordingListener final : public OrderBookListener {
public:
    void on_added(const OrderAdded& e) noexcept override { events.emplace_back(e); }
    void on_reduced(const OrderReduced& e) noexcept override { events.emplace_back(e); }
    void on_removed(const OrderRemoved& e) noexcept override { events.emplace_back(e); }
    void on_rejected(const OrderRejected& e) noexcept override { events.emplace_back(e); }

    std::vector<BookEvent> events;
};

} // namespace

// ---------- Shared setup ----------
// A book with a recording listener attached and no orders. The listener is declared first so it
// exists before the book that points to it (members are built in declaration order).
class EventBook : public ::testing::Test {
protected:
    RecordingListener recorder;
    OrderBook book{recorder};
};

// ---------- Changes to the book ----------

TEST_F(EventBook, AddSendsOrderAdded) {
    ASSERT_EQ(book.add_order(OrderId{1}, Side::Buy, Price{18742}, Qty{100}, Timestamp{1ns}),
              AddResult::Accepted);

    const std::vector<BookEvent> expected{
        OrderAdded{.seq = 1, .id = OrderId{1}, .side = Side::Buy, .price = Price{18742},
                   .qty = Qty{100},
                   .last = true},
    };
    EXPECT_EQ(recorder.events, expected);
}

// Sequence numbers count every event of this book, from 1, with no gaps.
TEST_F(EventBook, SequenceNumbersCountEveryEventFromOne) {
    ASSERT_EQ(book.add_order(OrderId{1}, Side::Buy, Price{18742}, Qty{100}, Timestamp{1ns}),
              AddResult::Accepted);
    ASSERT_EQ(book.add_order(OrderId{10}, Side::Sell, Price{18745}, Qty{75}, Timestamp{2ns}),
              AddResult::Accepted);
    ASSERT_EQ(book.cancel_order(OrderId{1}), CancelResult::Cancelled);

    const std::vector<BookEvent> expected{
        OrderAdded{.seq = 1, .id = OrderId{1}, .side = Side::Buy, .price = Price{18742},
                   .qty = Qty{100},
                   .last = true},
        OrderAdded{.seq = 2, .id = OrderId{10}, .side = Side::Sell, .price = Price{18745},
                   .qty = Qty{75},
                   .last = true},
        OrderRemoved{.seq = 3, .id = OrderId{1}, .side = Side::Buy, .price = Price{18742},
                     .qty = Qty{100},
                     .reason = RemoveReason::Cancelled, .last = true},
    };
    EXPECT_EQ(recorder.events, expected);
}

// 100 - 40 = 60 remain; the event carries both what remains and what was taken off.
TEST_F(EventBook, PartialReduceSendsReduced) {
    ASSERT_EQ(book.add_order(OrderId{1}, Side::Buy, Price{18742}, Qty{100}, Timestamp{1ns}),
              AddResult::Accepted);
    ASSERT_EQ(book.reduce_order(OrderId{1}, Qty{40}), ReduceResult::Reduced);

    const std::vector<BookEvent> expected{
        OrderAdded{.seq = 1, .id = OrderId{1}, .side = Side::Buy, .price = Price{18742},
                   .qty = Qty{100},
                   .last = true},
        OrderReduced{.seq = 2, .id = OrderId{1}, .side = Side::Buy, .price = Price{18742},
                     .qty = Qty{60},
                     .by = Qty{40}, .last = true},
    };
    EXPECT_EQ(recorder.events, expected);
}

// A cancel reports what was still resting (70 after a reduce of 30), not the original 100.
TEST_F(EventBook, CancelReportsTheQuantityThatLeftTheBook) {
    ASSERT_EQ(book.add_order(OrderId{1}, Side::Buy, Price{18742}, Qty{100}, Timestamp{1ns}),
              AddResult::Accepted);
    ASSERT_EQ(book.reduce_order(OrderId{1}, Qty{30}), ReduceResult::Reduced);
    ASSERT_EQ(book.cancel_order(OrderId{1}), CancelResult::Cancelled);

    const std::vector<BookEvent> expected{
        OrderAdded{.seq = 1, .id = OrderId{1}, .side = Side::Buy, .price = Price{18742},
                   .qty = Qty{100},
                   .last = true},
        OrderReduced{.seq = 2, .id = OrderId{1}, .side = Side::Buy, .price = Price{18742},
                     .qty = Qty{70},
                     .by = Qty{30}, .last = true},
        OrderRemoved{.seq = 3, .id = OrderId{1}, .side = Side::Buy, .price = Price{18742},
                     .qty = Qty{70},
                     .reason = RemoveReason::Cancelled, .last = true},
    };
    EXPECT_EQ(recorder.events, expected);
}

// Taking more than remains (150 of 100) removes the order; qty reports the 100 that left the book.
TEST_F(EventBook, ReduceTakingTheWholeOrderSendsRemoved) {
    ASSERT_EQ(book.add_order(OrderId{1}, Side::Buy, Price{18742}, Qty{100}, Timestamp{1ns}),
              AddResult::Accepted);
    ASSERT_EQ(book.reduce_order(OrderId{1}, Qty{150}), ReduceResult::Removed);

    const std::vector<BookEvent> expected{
        OrderAdded{.seq = 1, .id = OrderId{1}, .side = Side::Buy, .price = Price{18742},
                   .qty = Qty{100}, .last = true},
        OrderRemoved{.seq = 2, .id = OrderId{1}, .side = Side::Buy, .price = Price{18742},
                     .qty = Qty{100}, .reason = RemoveReason::Reduced, .last = true},
    };
    EXPECT_EQ(recorder.events, expected);
}

// Same price, 100 -> 60: the order keeps its place, so this is a reduction by 40, not a removal.
TEST_F(EventBook, ModifyDownAtSamePriceSendsReduced) {
    ASSERT_EQ(book.add_order(OrderId{1}, Side::Buy, Price{18742}, Qty{100}, Timestamp{1ns}),
              AddResult::Accepted);
    ASSERT_EQ(book.modify_order(OrderId{1}, Price{18742}, Qty{60}, Timestamp{2ns}),
              ModifyResult::Modified);

    const std::vector<BookEvent> expected{
        OrderAdded{.seq = 1, .id = OrderId{1}, .side = Side::Buy, .price = Price{18742},
                   .qty = Qty{100},
                   .last = true},
        OrderReduced{.seq = 2, .id = OrderId{1}, .side = Side::Buy, .price = Price{18742},
                     .qty = Qty{60},
                     .by = Qty{40}, .last = true},
    };
    EXPECT_EQ(recorder.events, expected);
}

// Same price and same quantity: the call succeeds but nothing changed, so nothing is sent.
TEST_F(EventBook, ModifyWithNoChangeSendsNothing) {
    ASSERT_EQ(book.add_order(OrderId{1}, Side::Buy, Price{18742}, Qty{100}, Timestamp{1ns}),
              AddResult::Accepted);
    ASSERT_EQ(book.modify_order(OrderId{1}, Price{18742}, Qty{100}, Timestamp{2ns}),
              ModifyResult::Modified);

    EXPECT_EQ(recorder.events.size(), 1u);  // only the Added from the setup line
}

// Same price, 100 -> 150: a larger quantity loses its place, so the order is removed and added
// again.
TEST_F(EventBook, ModifyUpSendsRemovedThenAdded) {
    ASSERT_EQ(book.add_order(OrderId{1}, Side::Buy, Price{18742}, Qty{100}, Timestamp{1ns}),
              AddResult::Accepted);
    ASSERT_EQ(book.modify_order(OrderId{1}, Price{18742}, Qty{150}, Timestamp{2ns}),
              ModifyResult::Modified);

    const std::vector<BookEvent> expected{
        OrderAdded{.seq = 1, .id = OrderId{1}, .side = Side::Buy, .price = Price{18742},
                   .qty = Qty{100}, .last = true},
        OrderRemoved{.seq = 2, .id = OrderId{1}, .side = Side::Buy, .price = Price{18742},
                     .qty = Qty{100}, .reason = RemoveReason::Modified, .last = false},
        OrderAdded{.seq = 3, .id = OrderId{1}, .side = Side::Buy, .price = Price{18742},
                   .qty = Qty{150}, .last = true},
    };
    EXPECT_EQ(recorder.events, expected);
}

// The sell side, and a price change: removed at the old price, then added at the new one.
TEST_F(EventBook, ModifyToNewPriceSendsRemovedThenAdded) {
    ASSERT_EQ(book.add_order(OrderId{10}, Side::Sell, Price{18745}, Qty{75}, Timestamp{1ns}),
              AddResult::Accepted);
    ASSERT_EQ(book.modify_order(OrderId{10}, Price{18744}, Qty{75}, Timestamp{2ns}),
              ModifyResult::Modified);

    const std::vector<BookEvent> expected{
        OrderAdded{.seq = 1, .id = OrderId{10}, .side = Side::Sell, .price = Price{18745},
                   .qty = Qty{75},
                   .last = true},
        OrderRemoved{.seq = 2, .id = OrderId{10}, .side = Side::Sell, .price = Price{18745},
                     .qty = Qty{75},
                     .reason = RemoveReason::Modified, .last = false},
        OrderAdded{.seq = 3, .id = OrderId{10}, .side = Side::Sell, .price = Price{18744},
                   .qty = Qty{75},
                   .last = true},
    };
    EXPECT_EQ(recorder.events, expected);
}

// Replace: the old id leaves (reason Replaced), the new id arrives with the new price and quantity.
TEST_F(EventBook, ReplaceSendsRemovedOldThenAddedNew) {
    ASSERT_EQ(book.add_order(OrderId{1}, Side::Buy, Price{18742}, Qty{100}, Timestamp{1ns}),
              AddResult::Accepted);
    ASSERT_EQ(book.replace_order(OrderId{1}, OrderId{20}, Price{18743}, Qty{80}, Timestamp{2ns}),
              ReplaceResult::Replaced);

    const std::vector<BookEvent> expected{
        OrderAdded{.seq = 1, .id = OrderId{1}, .side = Side::Buy, .price = Price{18742},
                   .qty = Qty{100},
                   .last = true},
        OrderRemoved{.seq = 2, .id = OrderId{1}, .side = Side::Buy, .price = Price{18742},
                     .qty = Qty{100},
                     .reason = RemoveReason::Replaced, .last = false},
        OrderAdded{.seq = 3, .id = OrderId{20}, .side = Side::Buy, .price = Price{18743},
                   .qty = Qty{80},
                   .last = true},
    };
    EXPECT_EQ(recorder.events, expected);
}

// A replace may keep the same id (the book allows it); the events look the same with one id.
TEST_F(EventBook, ReplaceKeepingTheSameIdSendsRemovedThenAdded) {
    ASSERT_EQ(book.add_order(OrderId{1}, Side::Buy, Price{18742}, Qty{100}, Timestamp{1ns}),
              AddResult::Accepted);
    ASSERT_EQ(book.replace_order(OrderId{1}, OrderId{1}, Price{18741}, Qty{50}, Timestamp{2ns}),
              ReplaceResult::Replaced);

    const std::vector<BookEvent> expected{
        OrderAdded{.seq = 1, .id = OrderId{1}, .side = Side::Buy, .price = Price{18742},
                   .qty = Qty{100},
                   .last = true},
        OrderRemoved{.seq = 2, .id = OrderId{1}, .side = Side::Buy, .price = Price{18742},
                     .qty = Qty{100},
                     .reason = RemoveReason::Replaced, .last = false},
        OrderAdded{.seq = 3, .id = OrderId{1}, .side = Side::Buy, .price = Price{18741},
                   .qty = Qty{50},
                   .last = true},
    };
    EXPECT_EQ(recorder.events, expected);
}

// ---------- Rejections: exactly one event, and the book is unchanged ----------

TEST_F(EventBook, RejectedAddSendsOneRejectedAndChangesNothing) {
    ASSERT_EQ(book.add_order(OrderId{1}, Side::Buy, Price{18742}, Qty{100}, Timestamp{1ns}),
              AddResult::Accepted);
    ASSERT_EQ(book.add_order(OrderId{1}, Side::Sell, Price{18750}, Qty{10}, Timestamp{2ns}),
              AddResult::DuplicateOrderId);

    const std::vector<BookEvent> expected{
        OrderAdded{.seq = 1, .id = OrderId{1}, .side = Side::Buy, .price = Price{18742},
                   .qty = Qty{100},
                   .last = true},
        OrderRejected{.seq = 2, .operation = Operation::Add, .id = OrderId{1},
                      .reason = AddResult::DuplicateOrderId, .last = true},
    };
    EXPECT_EQ(recorder.events, expected);

    const auto order = book.find_order(OrderId{1});
    ASSERT_TRUE(order.has_value());
    EXPECT_EQ(order->side, Side::Buy);
    EXPECT_EQ(order->qty, Qty{100});
    EXPECT_EQ(book.depth_at(Side::Sell, Price{18750}), Qty{0});
    EXPECT_FALSE(book.check_invariants().has_value());
}

TEST_F(EventBook, RejectedAddWithZeroQuantity) {
    ASSERT_EQ(book.add_order(OrderId{5}, Side::Buy, Price{18742}, Qty{0}, Timestamp{1ns}),
              AddResult::ZeroQuantity);

    const std::vector<BookEvent> expected{
        OrderRejected{.seq = 1, .operation = Operation::Add, .id = OrderId{5},
                      .reason = AddResult::ZeroQuantity, .last = true},
    };
    EXPECT_EQ(recorder.events, expected);
    EXPECT_FALSE(book.best_bid().has_value());
}

// The id may be unknown, so a rejection carries no side or price: only what the call named.
TEST_F(EventBook, RejectedCancelOfUnknownId) {
    ASSERT_EQ(book.cancel_order(OrderId{99}), CancelResult::UnknownOrderId);

    const std::vector<BookEvent> expected{
        OrderRejected{.seq = 1, .operation = Operation::Cancel, .id = OrderId{99},
                      .reason = CancelResult::UnknownOrderId, .last = true},
    };
    EXPECT_EQ(recorder.events, expected);
}

TEST_F(EventBook, RejectedModifies) {
    ASSERT_EQ(book.add_order(OrderId{1}, Side::Buy, Price{18742}, Qty{100}, Timestamp{1ns}),
              AddResult::Accepted);
    ASSERT_EQ(book.modify_order(OrderId{99}, Price{18742}, Qty{50}, Timestamp{2ns}),
              ModifyResult::UnknownOrderId);
    ASSERT_EQ(book.modify_order(OrderId{1}, Price{18742}, Qty{0}, Timestamp{3ns}),
              ModifyResult::ZeroQuantity);

    const std::vector<BookEvent> expected{
        OrderAdded{.seq = 1, .id = OrderId{1}, .side = Side::Buy, .price = Price{18742},
                   .qty = Qty{100},
                   .last = true},
        OrderRejected{.seq = 2, .operation = Operation::Modify, .id = OrderId{99},
                      .reason = ModifyResult::UnknownOrderId, .last = true},
        OrderRejected{.seq = 3, .operation = Operation::Modify, .id = OrderId{1},
                      .reason = ModifyResult::ZeroQuantity, .last = true},
    };
    EXPECT_EQ(recorder.events, expected);
    EXPECT_EQ(book.depth_at(Side::Buy, Price{18742}), Qty{100});
}

TEST_F(EventBook, RejectedReduces) {
    ASSERT_EQ(book.add_order(OrderId{1}, Side::Buy, Price{18742}, Qty{100}, Timestamp{1ns}),
              AddResult::Accepted);
    ASSERT_EQ(book.reduce_order(OrderId{99}, Qty{10}), ReduceResult::UnknownOrderId);
    ASSERT_EQ(book.reduce_order(OrderId{1}, Qty{0}), ReduceResult::ZeroQuantity);

    const std::vector<BookEvent> expected{
        OrderAdded{.seq = 1, .id = OrderId{1}, .side = Side::Buy, .price = Price{18742},
                   .qty = Qty{100},
                   .last = true},
        OrderRejected{.seq = 2, .operation = Operation::Reduce, .id = OrderId{99},
                      .reason = ReduceResult::UnknownOrderId, .last = true},
        OrderRejected{.seq = 3, .operation = Operation::Reduce, .id = OrderId{1},
                      .reason = ReduceResult::ZeroQuantity, .last = true},
    };
    EXPECT_EQ(recorder.events, expected);
    EXPECT_EQ(book.depth_at(Side::Buy, Price{18742}), Qty{100});
}

// For replace, the rejection names the old id (the call's first id), whichever id caused it.
TEST_F(EventBook, RejectedReplaces) {
    ASSERT_EQ(book.add_order(OrderId{1}, Side::Buy, Price{18742}, Qty{100}, Timestamp{1ns}),
              AddResult::Accepted);
    ASSERT_EQ(book.add_order(OrderId{2}, Side::Buy, Price{18742}, Qty{200}, Timestamp{2ns}),
              AddResult::Accepted);
    ASSERT_EQ(book.replace_order(OrderId{99}, OrderId{20}, Price{18743}, Qty{10}, Timestamp{3ns}),
              ReplaceResult::UnknownOrderId);
    ASSERT_EQ(book.replace_order(OrderId{1}, OrderId{2}, Price{18743}, Qty{10}, Timestamp{4ns}),
              ReplaceResult::DuplicateOrderId);
    ASSERT_EQ(book.replace_order(OrderId{1}, OrderId{20}, Price{18743}, Qty{0}, Timestamp{5ns}),
              ReplaceResult::ZeroQuantity);

    const std::vector<BookEvent> expected{
        OrderAdded{.seq = 1, .id = OrderId{1}, .side = Side::Buy, .price = Price{18742},
                   .qty = Qty{100},
                   .last = true},
        OrderAdded{.seq = 2, .id = OrderId{2}, .side = Side::Buy, .price = Price{18742},
                   .qty = Qty{200},
                   .last = true},
        OrderRejected{.seq = 3, .operation = Operation::Replace, .id = OrderId{99},
                      .reason = ReplaceResult::UnknownOrderId, .last = true},
        OrderRejected{.seq = 4, .operation = Operation::Replace, .id = OrderId{1},
                      .reason = ReplaceResult::DuplicateOrderId, .last = true},
        OrderRejected{.seq = 5, .operation = Operation::Replace, .id = OrderId{1},
                      .reason = ReplaceResult::ZeroQuantity, .last = true},
    };
    EXPECT_EQ(recorder.events, expected);
    EXPECT_EQ(book.depth_at(Side::Buy, Price{18742}), Qty{300});
    EXPECT_EQ(book.depth_at(Side::Buy, Price{18743}), Qty{0});
    EXPECT_FALSE(book.check_invariants().has_value());
}

// ---------- Queries, timing, and the callback rule ----------

TEST_F(EventBook, QueriesSendNothing) {
    ASSERT_EQ(book.add_order(OrderId{1}, Side::Buy, Price{18742}, Qty{100}, Timestamp{1ns}),
              AddResult::Accepted);

    (void)book.find_order(OrderId{1});
    (void)book.find_order(OrderId{99});
    (void)book.queue_ahead(OrderId{1});
    (void)book.best_bid();
    (void)book.best_ask();
    (void)book.depth_at(Side::Buy, Price{18742});
    (void)book.check_invariants();

    EXPECT_EQ(recorder.events.size(), 1u);  // only the Added
}

namespace {
// Asks the book for the depth at the event's price from inside the callback.
class DepthProbe final : public OrderBookListener {
public:
    void on_added(const OrderAdded& e) noexcept override {
        depth_seen.push_back(book->depth_at(e.side, e.price));
    }
    void on_removed(const OrderRemoved& e) noexcept override {
        depth_seen.push_back(book->depth_at(e.side, e.price));
    }

    const OrderBook* book = nullptr;
    std::vector<Qty> depth_seen;
};
} // namespace

// Events arrive after the change is complete: the listener already sees the new depth.
// 100 after the first add, 300 after the second, 200 after cancelling the first.
TEST(EventTiming, ListenerSeesTheBookAfterTheChange) {
    DepthProbe probe;
    OrderBook book{probe};
    probe.book = &book;

    ASSERT_EQ(book.add_order(OrderId{1}, Side::Buy, Price{18742}, Qty{100}, Timestamp{1ns}),
              AddResult::Accepted);
    ASSERT_EQ(book.add_order(OrderId{2}, Side::Buy, Price{18742}, Qty{200}, Timestamp{2ns}),
              AddResult::Accepted);
    ASSERT_EQ(book.cancel_order(OrderId{1}), CancelResult::Cancelled);

    const std::vector<Qty> expected{Qty{100}, Qty{300}, Qty{200}};
    EXPECT_EQ(probe.depth_seen, expected);
}

namespace {
// Breaks the rule on purpose: changes the book from inside a callback.
class CallsBackIn final : public OrderBookListener {
public:
    void on_added(const OrderAdded& e) noexcept override { (void)book->cancel_order(e.id); }

    OrderBook* book = nullptr;
};
} // namespace

// Debug builds stop with the assert's message. Release builds (asserts off) only run the statement,
// which EXPECT_DEBUG_DEATH allows.
TEST(ListenerRules, CallingBackIntoTheBookIsCaughtInDebugBuilds) {
    CallsBackIn listener;
    OrderBook book{listener};
    listener.book = &book;

    EXPECT_DEBUG_DEATH(
        (void)book.add_order(OrderId{1}, Side::Buy, Price{18742}, Qty{100}, Timestamp{1ns}),
        "Listener must not call back into the order book"
    );
}

// ---------- The stream is complete: a mirror built from events alone matches the book ----------
// Idea from R0141 (intrepidkarthi/orderbook, TestEventStreamReconstructsBook): if any event were
// missing or wrong, depth rebuilt from the events would drift from the book's own depth.

namespace {
// Rebuilds depth per (side, price) from events only, and counts calls whose last event arrived.
class MirrorListener final : public OrderBookListener {
public:
    void on_added(const OrderAdded& e) noexcept override {
        depth[{e.side, e.price.value()}] += e.qty.value();
        count_end(e.last);
    }
    void on_reduced(const OrderReduced& e) noexcept override {
        depth[{e.side, e.price.value()}] -= e.by.value();
        count_end(e.last);
    }
    void on_removed(const OrderRemoved& e) noexcept override {
        depth[{e.side, e.price.value()}] -= e.qty.value();
        count_end(e.last);
    }
    void on_rejected(const OrderRejected& e) noexcept override { count_end(e.last); }

    std::map<std::pair<Side, std::int64_t>, std::int64_t> depth;
    int calls_ended = 0;

private:
    void count_end(bool last) noexcept {
        if (last) ++calls_ended;
    }
};
} // namespace

// After every call: the mirror's depth equals the book's at every price used, and exactly one
// more call has ended (each call below sends at least one event, and its final event carries
// last = true).
TEST(EventStream, MirrorRebuiltFromEventsMatchesTheBookAfterEveryCall) {
    MirrorListener mirror;
    OrderBook book{mirror};
    int calls = 0;

    const auto check = [&] {
        ++calls;
        EXPECT_EQ(mirror.calls_ended, calls) << "after call " << calls;
        for (const Side side : {Side::Buy, Side::Sell}) {
            for (std::int64_t price = 18738; price <= 18748; ++price) {
                const auto it = mirror.depth.find({side, price});
                const std::int64_t mirrored = (it == mirror.depth.end()) ? 0 : it->second;
                EXPECT_EQ(book.depth_at(side, Price{price}).value(), mirrored)
                    << "after call " << calls << ", side " << static_cast<int>(side)
                    << ", price " << price;
            }
        }
    };

    ASSERT_EQ(book.add_order(OrderId{1}, Side::Buy, Price{18742}, Qty{100}, Timestamp{1ns}),
              AddResult::Accepted);
    check();
    ASSERT_EQ(book.add_order(OrderId{2}, Side::Buy, Price{18742}, Qty{200}, Timestamp{2ns}),
              AddResult::Accepted);
    check();
    ASSERT_EQ(book.add_order(OrderId{10}, Side::Sell, Price{18745}, Qty{75}, Timestamp{3ns}),
              AddResult::Accepted);
    check();
    ASSERT_EQ(book.modify_order(OrderId{1}, Price{18742}, Qty{60}, Timestamp{4ns}),
              ModifyResult::Modified);
    check();  // down in place
    ASSERT_EQ(book.modify_order(OrderId{2}, Price{18742}, Qty{250}, Timestamp{5ns}),
              ModifyResult::Modified);
    check();  // up: loses place
    ASSERT_EQ(book.modify_order(OrderId{10}, Price{18746}, Qty{75}, Timestamp{6ns}),
              ModifyResult::Modified);
    check();  // new price
    ASSERT_EQ(book.reduce_order(OrderId{1}, Qty{10}), ReduceResult::Reduced);
    check();
    ASSERT_EQ(book.replace_order(OrderId{2}, OrderId{20}, Price{18741}, Qty{120}, Timestamp{7ns}),
              ReplaceResult::Replaced);
    check();
    ASSERT_EQ(book.replace_order(OrderId{20}, OrderId{20}, Price{18743}, Qty{30}, Timestamp{8ns}),
              ReplaceResult::Replaced);
    check();  // same id
    ASSERT_EQ(book.add_order(OrderId{1}, Side::Sell, Price{18747}, Qty{5}, Timestamp{9ns}),
              AddResult::DuplicateOrderId);
    check();  // rejected: depth unchanged
    ASSERT_EQ(book.reduce_order(OrderId{1}, Qty{500}), ReduceResult::Removed);
    check();  // takes the whole order
    ASSERT_EQ(book.cancel_order(OrderId{10}), CancelResult::Cancelled);
    check();
    ASSERT_EQ(book.cancel_order(OrderId{10}), CancelResult::UnknownOrderId);
    check();  // rejected
}

