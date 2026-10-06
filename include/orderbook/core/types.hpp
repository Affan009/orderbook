#pragma once

#include <cassert>
#include <chrono>
#include <compare>
#include <cstdint>

namespace orderbook::core {

// StrongType template to differentiate among some whole-
// number values by adding a Tag.
// Tag itself is never used inside the template.
// This way Price cannot be passed where OrderId is expected.
template <typename Tag>
class StrongType {
public:
    // Constructor is explicit to avoid silent conversions.
    constexpr explicit StrongType(std::int64_t value) : value_{value} {}
    constexpr std::int64_t value() const { return value_; }
    constexpr auto operator<=>(const StrongType&) const = default;

private:
    std::int64_t value_;
};

// Price type counts the number of ticks, and not currencies (dollars or rupees).
using Price = StrongType<struct PriceTag>;

// OrderId type works as an identifier for orders.
// They are used for cancelling or modifying an order.
using OrderId = StrongType<struct OrderIdTag>;

// Qty type counts the number of lots. It is never negative!
class Qty {
public:
    constexpr explicit Qty(std::int64_t value) : value_{value} {
        // Only checked in debug builds, and removed in release preset.
        assert(value >= 0 && "Quantity must always be non-negative");
    }
    constexpr std::int64_t value() const { return value_; }
    constexpr auto operator<=>(const Qty&) const = default;

private:
    std::int64_t value_;
};

// Side clarifies the side of the order and what it can be matched to.
enum class Side { Buy, Sell };

// Timestamp is the arrival time of an order, measured in nanoseconds.
// The exchange provides this to the orderbook.
using Timestamp = std::chrono::sys_time<std::chrono::nanoseconds>;

} // namespace orderbook::core
