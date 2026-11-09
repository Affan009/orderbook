#pragma once

#include <cstdint>
#include <functional>
#include <iosfwd>
#include <map>
#include <optional>
#include <unordered_map>

#include "orderbook/book/level.hpp"
#include "orderbook/book/listener.hpp"
#include "orderbook/book/order.hpp"
#include "orderbook/book/results.hpp"
#include "orderbook/core/types.hpp"

namespace orderbook::book {

// Tells which rule check_invariants() found broken first.
enum class InvariantViolation : std::uint8_t {
    EmptyLevel, NonPositiveQty, WrongSide, WrongPrice,
    IndexMismatch, DepthMismatch, CountMismatch
};

// Orderbook structure that collects and sorts bids and asks according to price level.
class OrderBook {
public:
    OrderBook();
    explicit OrderBook(OrderBookListener& listener);

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
    [[nodiscard]] std::optional<InvariantViolation> check_invariants() const;

private:
    friend struct OrderBookTestAccess;

    // Storage changes only; these never send events. Every public function goes through them.
    // Puts an order at the back of its price level's queue and indexes it.
    void insert_resting(const Order& order);

    // Takes an order out of its level and the index, erasing the level if it empties.
    // Returns the removed order.
    Order remove_resting(Level::iterator order_it);

    // Shrinks a resting order by `by` (less than its quantity); it keeps its place.
    // Returns the order after the cut.
    Order reduce_resting(Level::iterator order_it, core::Qty by);

    // Hands one event to the listener, with in_callback_ set for the duration of the call.
    template <typename Event>
    void send(const Event& event);

    // Sends a rejection (always the last event of its call) and returns `reason`
    // for the caller to return.
    template <typename Result>
    Result reject(Operation operation, core::OrderId id, Result reason);

    OrderBookListener* listener_;  // never null: the shared NullListener when none is given
    std::int64_t next_seq_{1};     // sequence number of the next event; the first event is 1
    bool in_callback_{false};      // true while a listener function runs and catches calls back in
                                   // (not a thread-safety mechanism)

    std::map<core::Price, Level, std::greater<>> bids_;
    std::map<core::Price, Level> asks_;
    std::unordered_map<core::OrderId, Level::iterator> orders_by_id_;
};

} // namespace orderbook::book
