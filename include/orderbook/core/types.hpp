#pragma once

#include <compare>
#include <cstdint>

namespace orderbook::core {

class Price {
public:
    constexpr explicit Price(std::int64_t value) : value_{value} {}
    constexpr std::int64_t value() const { return value_; }
    constexpr auto operator<=>(const Price&) const = default;

private:
    std::int64_t value_;
};

} // namespace orderbook::core
