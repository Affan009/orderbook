#pragma once

#include <cstdint>
#include <variant>

#include "orderbook/book/results.hpp"
#include "orderbook/core/types.hpp"

namespace orderbook::book {

// Tells the reason an order was removed.
enum class RemoveReason : std::uint8_t { Cancelled, Modified, Reduced, Replaced };

// Tells which call a rejection came from.
enum class Operation : std::uint8_t { Add, Cancel, Modify, Reduce, Replace };

// Holds the result value the rejected call returned: one of the five result enums.
using RejectReason = std::variant<AddResult, CancelResult, ModifyResult,
                                  ReduceResult, ReplaceResult>;

// An order joined the book.
struct OrderAdded {
    std::uint64_t seq;
    core::OrderId id;
    core::Side side;
    core::Price price;
    core::Qty qty;
    bool last;

    bool operator==(const OrderAdded&) const = default;
};

// An order's quantity went down; it keeps its place in the queue.
struct OrderReduced {
    std::uint64_t seq;
    core::OrderId id;
    core::Side side;
    core::Price price;
    core::Qty qty;    // what quantity remains in the book
    core::Qty by;     // what was taken off
    bool last;

    bool operator==(const OrderReduced&) const = default;
};

// An order left the book.
struct OrderRemoved {
    std::uint64_t seq;
    core::OrderId id;
    core::Side side;
    core::Price price;
    core::Qty qty;
    RemoveReason reason;
    bool last;

    bool operator==(const OrderRemoved&) const = default;
};

// A call was refused; the book did not change.
struct OrderRejected {
    std::uint64_t seq;
    Operation operation;
    core::OrderId id;
    RejectReason reason;
    bool last;

    bool operator==(const OrderRejected&) const = default;
};

// Any one of the four events: for code that keeps or forwards a single stream of them.
using BookEvent = std::variant<OrderAdded, OrderReduced, OrderRemoved, OrderRejected>;

} // namespace orderbook::book
