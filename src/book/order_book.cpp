#include <cassert>
#include <optional>
#include <ostream>
#include <type_traits>

#include "orderbook/book/order_book.hpp"

namespace orderbook::book {

namespace {
// Shared by every book built without a listener; it has no state, so sharing is safe.
NullListener null_listener;
} // namespace

OrderBook::OrderBook() : listener_{&null_listener} {}
OrderBook::OrderBook(OrderBookListener& listener) : listener_{&listener} {}

template <typename Event>
void OrderBook::send(const Event& event) {
    in_callback_ = true;
    if constexpr (std::is_same_v<Event, OrderAdded>) {
        listener_->on_added(event);
    } else if constexpr (std::is_same_v<Event, OrderRemoved>) {
        listener_->on_removed(event);
    } else if constexpr (std::is_same_v<Event, OrderReduced>) {
        listener_->on_reduced(event);
    } else {
        static_assert(std::is_same_v<Event, OrderRejected>, "send: unknown event type");
        listener_->on_rejected(event);
    }
    in_callback_ = false;
}

template <typename Result>
Result OrderBook::reject(Operation operation, core::OrderId id, Result reason) {
    send(OrderRejected{.seq = next_seq_++, .operation = operation, .id = id,
                       .reason = reason, .last = true});
    return reason;
}

void OrderBook::insert_resting(const Order& order) {
    auto& price_level = (order.side == core::Side::Buy) ? bids_[order.price] : asks_[order.price];
    auto it = price_level.add(order);
    orders_by_id_.emplace(order.id, it);
}

Order OrderBook::remove_resting(Level::iterator order_it) {
    const Order removed = *order_it;  // copy first: order_it is invalid once the order is erased

    auto& price_level = (removed.side == core::Side::Buy) ? bids_[removed.price]
                                                          : asks_[removed.price];
    price_level.remove(order_it);

    if (price_level.empty()) {
        if (removed.side == core::Side::Buy) bids_.erase(removed.price);
        else asks_.erase(removed.price);
    }

    orders_by_id_.erase(removed.id);
    return removed;
}

Order OrderBook::reduce_resting(Level::iterator order_it, core::Qty by) {
    auto& price_level = (order_it->side == core::Side::Buy) ? bids_[order_it->price]
                                                            : asks_[order_it->price];
    price_level.reduce(order_it, by);
    return *order_it;
}

AddResult OrderBook::add_order(
    core::OrderId id,
    core::Side side,
    core::Price price,
    core::Qty qty,
    core::Timestamp arrival_time
) {
    assert(!in_callback_ && "Listener must not call back into the order book");

    if (qty == core::Qty{0}) return reject(Operation::Add, id, AddResult::ZeroQuantity);
    if (orders_by_id_.contains(id)) {
        return reject(Operation::Add, id, AddResult::DuplicateOrderId);
    }

    insert_resting(Order{id, side, price, qty, arrival_time});
    send(OrderAdded{.seq = next_seq_++, .id = id, .side = side, .price = price,
                    .qty = qty, .last = true});

    return AddResult::Accepted;
}

CancelResult OrderBook::cancel_order(core::OrderId id) {
    assert(!in_callback_ && "Listener must not call back into the order book");

    const auto map_it = orders_by_id_.find(id);
    if (map_it == orders_by_id_.end()) {
        return reject(Operation::Cancel, id, CancelResult::UnknownOrderId);
    }

    const Order removed = remove_resting(map_it->second);
    send(OrderRemoved{.seq = next_seq_++, .id = removed.id, .side = removed.side,
                      .price = removed.price, .qty = removed.qty,
                      .reason = RemoveReason::Cancelled, .last = true});
    return CancelResult::Cancelled;
}

ModifyResult OrderBook::modify_order(
    core::OrderId id,
    core::Price new_price,
    core::Qty new_qty,
    core::Timestamp time
) {
    assert(!in_callback_ && "Listener must not call back into the order book");

    const auto map_it = orders_by_id_.find(id);
    if (map_it == orders_by_id_.end()) {
        return reject(Operation::Modify, id, ModifyResult::UnknownOrderId);
    }
    if (new_qty == core::Qty{0}) return reject(Operation::Modify, id, ModifyResult::ZeroQuantity);

    const auto order_it = map_it->second;

    const core::Side side = order_it->side;
    const core::Price price = order_it->price;
    const core::Qty qty = order_it->qty;

    if (new_price == price && new_qty <= qty) {
        // Same price and not larger: the order keeps its place in the queue.
        if (new_qty < qty) {
            const core::Qty by = qty - new_qty;
            const Order reduced = reduce_resting(order_it, by);
            send(OrderReduced{.seq = next_seq_++, .id = reduced.id, .side = reduced.side,
                              .price = reduced.price, .qty = reduced.qty, .by = by, .last = true});
        }
        // (new_qty == qty changes nothing, so nothing is sent.)
    } else {
        // Larger, or a new price: it loses its place, so it leaves and re-enters at the back.
        const Order removed = remove_resting(order_it);
        send(OrderRemoved{.seq = next_seq_++, .id = removed.id, .side = removed.side,
                          .price = removed.price, .qty = removed.qty,
                          .reason = RemoveReason::Modified, .last = false});

        insert_resting(Order{id, side, new_price, new_qty, time});
        send(OrderAdded{.seq = next_seq_++, .id = id, .side = side,
                        .price = new_price, .qty = new_qty, .last = true});
    }

    return ModifyResult::Modified;
}

ReduceResult OrderBook::reduce_order(core::OrderId id, core::Qty by) {
    assert(!in_callback_ && "Listener must not call back into the order book");

    const auto map_it = orders_by_id_.find(id);
    if (map_it == orders_by_id_.end()) {
        return reject(Operation::Reduce, id, ReduceResult::UnknownOrderId);
    }
    if (by == core::Qty{0}) return reject(Operation::Reduce, id, ReduceResult::ZeroQuantity);

    const auto order_it = map_it->second;

    // Taking at least what remains removes the order (the feed's "partial cancel" can take it all).
    if (by >= order_it->qty) {
        const Order removed = remove_resting(order_it);
        send(OrderRemoved{.seq = next_seq_++, .id = removed.id, .side = removed.side,
                          .price = removed.price, .qty = removed.qty,
                          .reason = RemoveReason::Reduced, .last = true});
        return ReduceResult::Removed;
    }

    const Order reduced = reduce_resting(order_it, by);
    send(OrderReduced{.seq = next_seq_++, .id = reduced.id, .side = reduced.side,
                      .price = reduced.price, .qty = reduced.qty, .by = by, .last = true});
    return ReduceResult::Reduced;
}

ReplaceResult OrderBook::replace_order(
    core::OrderId old_id,
    core::OrderId new_id,
    core::Price new_price,
    core::Qty new_qty,
    core::Timestamp time
) {
    assert(!in_callback_ && "Listener must not call back into the order book");

    const auto map_it = orders_by_id_.find(old_id);
    if (map_it == orders_by_id_.end()) {
        return reject(Operation::Replace, old_id, ReplaceResult::UnknownOrderId);
    }
    if (new_qty == core::Qty{0}) {
        return reject(Operation::Replace, old_id, ReplaceResult::ZeroQuantity);
    }
    if (new_id != old_id && orders_by_id_.contains(new_id)) {
        return reject(Operation::Replace, old_id, ReplaceResult::DuplicateOrderId);
    }

    const auto order_it = map_it->second;

    // Remove before insert: when new_id == old_id, the old index entry must be gone first.
    const Order removed = remove_resting(order_it);
    send(OrderRemoved{.seq = next_seq_++, .id = removed.id, .side = removed.side,
                      .price = removed.price, .qty = removed.qty,
                      .reason = RemoveReason::Replaced, .last = false});

    insert_resting(Order{new_id, removed.side, new_price, new_qty, time});
    send(OrderAdded{.seq = next_seq_++, .id = new_id, .side = removed.side, .price = new_price,
                    .qty = new_qty, .last = true});

    return ReplaceResult::Replaced;
}

std::optional<Order> OrderBook::find_order(core::OrderId id) const {
    const auto map_it = orders_by_id_.find(id);
    if (map_it == orders_by_id_.end()) return std::nullopt;

    return *(map_it->second);
}

std::optional<core::Qty> OrderBook::queue_ahead(core::OrderId id) const {
    const auto map_it = orders_by_id_.find(id);
    if (map_it == orders_by_id_.end()) return std::nullopt;

    const auto order_it = map_it->second;

    const core::Side side = order_it->side;
    const core::Price price = order_it->price;

    if (side == core::Side::Buy) {
        const auto level_it = bids_.find(price);
        return level_it->second.qty_ahead(order_it);
    }
    const auto level_it = asks_.find(price);
    return level_it->second.qty_ahead(order_it);
}

std::optional<core::Price> OrderBook::best_bid() const {
    if (bids_.empty()) return std::nullopt;
    return bids_.begin()->first;
}

std::optional<core::Price> OrderBook::best_ask() const {
    if (asks_.empty()) return std::nullopt;
    return asks_.begin()->first;
}

core::Qty OrderBook::depth_at(core::Side side, core::Price price) const {
    if (side == core::Side::Buy) {
        const auto level_it = bids_.find(price);
        if (level_it == bids_.end()) return core::Qty{0};
        return level_it->second.depth();
    }
    const auto level_it = asks_.find(price);
    if (level_it == asks_.end()) return core::Qty{0};
    return level_it->second.depth();
}

void OrderBook::print_book(std::ostream& out) const {
    // Asks print highest first, so the best prices on both sides meet at the spread line.
    for (auto it = asks_.rbegin(); it != asks_.rend(); ++it) {
        out << "ASK " << it->first.value() << " qty=" << it->second.depth().value()
            << " orders=" << it->second.order_count() << '\n';
    }

    if (!asks_.empty() && !bids_.empty()) {
        out << "--- spread " << best_ask()->value() - best_bid()->value()
            << " ---\n";
    }

    for (const auto& [price, level] : bids_) {
        out << "BID " << price.value() << " qty=" << level.depth().value()
            << " orders=" << level.order_count() << '\n';
    }
}

std::optional<InvariantViolation> OrderBook::check_invariants() const {
    std::size_t order_count{0};

    for(const auto& [price, level] : bids_) {
        if (level.empty()) return InvariantViolation::EmptyLevel;

        core::Qty agg_qty{0};
        for (auto it = level.begin(); it != level.end(); ++it) {
            if (it->qty == core::Qty{0}) return InvariantViolation::NonPositiveQty;
            else if (it->side != core::Side::Buy) return InvariantViolation::WrongSide;
            else if (it->price != price) return InvariantViolation::WrongPrice;

            const auto map_it = orders_by_id_.find(it->id);
            if (map_it == orders_by_id_.end() ||
                &(*map_it->second) != &(*it)) return InvariantViolation::IndexMismatch;

            agg_qty += it->qty;
            order_count++;
        }
        if (agg_qty != level.depth()) return InvariantViolation::DepthMismatch;
    }

    for(const auto& [price, level] : asks_) {
        if (level.empty()) return InvariantViolation::EmptyLevel;

        core::Qty agg_qty{0};
        for (auto it = level.begin(); it != level.end(); ++it) {
            if (it->qty == core::Qty{0}) return InvariantViolation::NonPositiveQty;
            else if (it->side != core::Side::Sell) return InvariantViolation::WrongSide;
            else if (it->price != price) return InvariantViolation::WrongPrice;

            const auto map_it = orders_by_id_.find(it->id);
            if (map_it == orders_by_id_.end() ||
                &(*map_it->second) != &(*it)) return InvariantViolation::IndexMismatch;

            agg_qty += it->qty;
            order_count++;
        }
        if (agg_qty != level.depth()) return InvariantViolation::DepthMismatch;
    }

    if (order_count != orders_by_id_.size()) return InvariantViolation::CountMismatch;

    return std::nullopt;
}

} // namespace orderbook::book
