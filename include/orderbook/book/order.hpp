#pragma once

#include "orderbook/core/types.hpp"

namespace orderbook::book {

// A resting order in a limit order book.
// Plain struct: each field enforces its own checks.
struct Order {
    core::OrderId id;
    core::Side side;
    core::Price price;
    core::Qty qty;
    core::Timestamp arrival_time;
};

} // namespace orderbook::book
