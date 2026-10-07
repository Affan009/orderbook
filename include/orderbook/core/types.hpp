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

// Qty type counts the number of lots in steps of lot size. It is never negative!
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

// How many stored units make one whole unit.
// It is used only when converting to or from real-world values (loading or displaying data).
// (The book never uses it, it only works in ticks and steps).
inline constexpr std::int64_t FIXED_POINT_SCALE = 1'000'000'000;

// TickSize is the size of one price step, counted in billionths of a currency unit.
// Ex: 0.01 currency unit = FIXED_POINT_SCALE / 100.
using TickSize = StrongType<struct TickSizeTag>;

// LotSize is the size of smallest tradable quantity, counted in billionths of a unit.
// Ex: 1 share = FIXED_POINT_SCALE, or 0.001 BTC step = FIXED_POINT_SCALE / 1000.
using LotSize = StrongType<struct LotSizeTag>;

} // namespace orderbook::core
