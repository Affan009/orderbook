#pragma once

#include "orderbook/book/events.hpp"

namespace orderbook::book {

// Receives every change and rejection from an OrderBook, after the change is complete.
// A listener must not call a state-changing book function from inside these calls.
class OrderBookListener {
public:
    virtual ~OrderBookListener() = default;

    virtual void on_added(const OrderAdded&) noexcept {}
    virtual void on_reduced(const OrderReduced&) noexcept {}
    virtual void on_removed(const OrderRemoved&) noexcept {}
    virtual void on_rejected(const OrderRejected&) noexcept {}
};

// The default listener: ignores everything.
class NullListener final : public OrderBookListener {};

} // namespace orderbook::book
