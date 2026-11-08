#include <cassert>
#include <optional>
#include <ostream>

#include "orderbook/book/order_book.hpp"

namespace orderbook::book {

AddResult OrderBook::add_order(
    core::OrderId id,
    core::Side side,
    core::Price price,
    core::Qty qty,
    core::Timestamp arrival_time
) {
    if (qty == core::Qty{0}) return AddResult::ZeroQuantity;
    if (orders_by_id_.contains(id)) return AddResult::DuplicateOrderId;

    auto& price_level = (side == core::Side::Buy) ? bids_[price] : asks_[price];
    auto it = price_level.add(Order{id, side, price, qty, arrival_time});

    orders_by_id_.emplace(id, it);

    return AddResult::Accepted;
}

CancelResult OrderBook::cancel_order(core::OrderId id) {
    const auto map_it = orders_by_id_.find(id);
    if (map_it == orders_by_id_.end()) return CancelResult::UnknownOrderId;

    const auto order_it = map_it->second;

    const core::Side side = order_it->side;
    const core::Price price = order_it->price;

    auto& price_level = (side == core::Side::Buy) ? bids_[price] : asks_[price];
    price_level.remove(order_it);

    if (price_level.empty()) {
        if (side == core::Side::Buy) bids_.erase(price);
        else asks_.erase(price);
    }

    orders_by_id_.erase(map_it);

    return CancelResult::Cancelled;
}

ModifyResult OrderBook::modify_order(
    core::OrderId id,
    core::Price new_price,
    core::Qty new_qty,
    core::Timestamp time
) {
    const auto map_it = orders_by_id_.find(id);
    if (map_it == orders_by_id_.end()) return ModifyResult::UnknownOrderId;
    if (new_qty == core::Qty{0}) return ModifyResult::ZeroQuantity;

    const auto order_it = map_it->second;

    const core::Side side = order_it->side;
    const core::Price price = order_it->price;
    const core::Qty qty = order_it->qty;

    if (new_price == price && new_qty <= qty) {
        if (new_qty < qty) {
            [[maybe_unused]] const auto reduce_result = reduce_order(id, qty - new_qty);
            assert(reduce_result == ReduceResult::Reduced);
        }
    } else {
        [[maybe_unused]] const auto cancel_result = cancel_order(id);
        assert(cancel_result == CancelResult::Cancelled);

        [[maybe_unused]] const auto add_result = add_order(id, side, new_price, new_qty, time);
        assert(add_result == AddResult::Accepted);
    }

    return ModifyResult::Modified;
}

ReduceResult OrderBook::reduce_order(core::OrderId id, core::Qty by) {
    const auto map_it = orders_by_id_.find(id);
    if (map_it == orders_by_id_.end()) return ReduceResult::UnknownOrderId;
    if (by == core::Qty{0}) return ReduceResult::ZeroQuantity;

    const auto order_it = map_it->second;

    if (by >= order_it->qty) {
        [[maybe_unused]] const auto result = cancel_order(id);
        assert(result == CancelResult::Cancelled);
        return ReduceResult::Removed;
    }

    const core::Side side = order_it->side;
    const core::Price price = order_it->price;

    auto& price_level = (side == core::Side::Buy) ? bids_[price] : asks_[price];
    price_level.reduce(order_it, by);

    return ReduceResult::Reduced;
}

ReplaceResult OrderBook::replace_order(
    core::OrderId old_id,
    core::OrderId new_id,
    core::Price new_price,
    core::Qty new_qty,
    core::Timestamp time
) {
    const auto map_it = orders_by_id_.find(old_id);
    if (map_it == orders_by_id_.end()) return ReplaceResult::UnknownOrderId;
    if (new_qty == core::Qty{0}) return ReplaceResult::ZeroQuantity;
    if (new_id != old_id && orders_by_id_.contains(new_id)) return ReplaceResult::DuplicateOrderId;

    const auto order_it = map_it->second;

    const core::Side side = order_it->side;

    [[maybe_unused]] const auto cancel_result = cancel_order(old_id);
    assert(cancel_result == CancelResult::Cancelled);

    [[maybe_unused]] const auto add_result = add_order(new_id, side, new_price, new_qty, time);
    assert(add_result == AddResult::Accepted);

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
