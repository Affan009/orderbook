#pragma once

#include <cassert>
#include <compare>
#include <cstdint>

namespace orderbook::core {

template <typename Tag>
class StrongType {
public:
    constexpr explicit StrongType(std::int64_t value) : value_{value} {}
    constexpr std::int64_t value() const { return value_; }
    constexpr auto operator<=>(const StrongType&) const = default;

private:
    std::int64_t value_;
};

using Price = StrongType<struct PriceTag>;
using OrderId = StrongType<struct OrderIdTag>;

class Qty {
public:
    constexpr explicit Qty(std::int64_t value) : value_{value} {
        assert(value >= 0 && "Quantity must always be non-negative");
    }
    constexpr std::int64_t value() const { return value_; }
    constexpr auto operator<=>(const Qty&) const = default;

private:
    std::int64_t value_;
};

enum class Side { Buy, Sell }; 

} // namespace orderbook::core
