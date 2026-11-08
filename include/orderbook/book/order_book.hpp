#pragma once

#include <cstdint>
#include <functional>
#include <iosfwd>
#include <map>
#include <optional>
#include <unordered_map>

#include "orderbook/book/level.hpp"
#include "orderbook/book/order.hpp"
#include "orderbook/core/types.hpp"

namespace orderbook::book {

// Tells the result of an order addition/cancellation to the book.
enum class AddResult : std::uint8_t { Accepted, DuplicateOrderId, ZeroQuantity };
enum class CancelResult : std::uint8_t { Cancelled, UnknownOrderId };

// Tells the result of an order modification/reduction/replacement to the book.
enum class ModifyResult : std::uint8_t { Modified, UnknownOrderId, ZeroQuantity };
enum class ReduceResult : std::uint8_t { Reduced, Removed, UnknownOrderId, ZeroQuantity };
enum class ReplaceResult : std::uint8_t { Replaced, UnknownOrderId, DuplicateOrderId, ZeroQuantity };

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
    [[nodiscard]] CancelResult cancel_order(core::OrderId id);
    [[nodiscard]] ModifyResult modify_order(
        core::OrderId id,
        core::Price new_price,
        core::Qty new_qty,
        core::Timestamp time
    );
    [[nodiscard]] ReduceResult reduce_order(core::OrderId id, core::Qty by);
    [[nodiscard]] ReplaceResult replace_order(
        core::OrderId old_id,
        core::OrderId new_id,
        core::Price new_price,
        core::Qty new_qty,
        core::Timestamp time
    );

    std::optional<Order> find_order(core::OrderId id) const;
    std::optional<core::Qty> queue_ahead(core::OrderId id) const;
    std::optional<core::Price> best_bid() const;
    std::optional<core::Price> best_ask() const;
    core::Qty depth_at(core::Side side, core::Price price) const;
    void print_book(std::ostream& out) const;

private:
    std::map<core::Price, Level, std::greater<>> bids_;
    std::map<core::Price, Level> asks_;
    std::unordered_map<core::OrderId, Level::iterator> orders_by_id_;
};

} // namespace orderbook::book
