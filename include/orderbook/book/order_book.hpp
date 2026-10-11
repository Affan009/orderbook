#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <unordered_map>

#include "orderbook/book/level.hpp"
#include "orderbook/book/order.hpp"
#include "orderbook/core/types.hpp"

namespace orderbook::book {

// Tells the result of the order addition to the book
enum class AddResult : std::uint8_t { Accepted, DuplicateOrderId, ZeroQuantity };

// Orderbook structure that collects and sorts bids and asks according to price level.
class OrderBook {
public:
    [[nodiscard]] AddResult add_order(
        core::OrderId id,
        core::Side side,
        core::Price price,
        core::Qty qty,
        core::Timestamp arrival_time
    );
    std::optional<core::Price> best_bid() const;
    std::optional<core::Price> best_ask() const;
    core::Qty depth_at(core::Side side, core::Price price) const;

private:
    std::map<core::Price, Level, std::greater<>> bids_;
    std::map<core::Price, Level> asks_;
    std::unordered_map<core::OrderId, Level::iterator> orders_by_id_;
};

} // namespace orderbook::book
