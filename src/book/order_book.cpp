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
        const auto it = bids_.find(price);
        if (it == bids_.end()) return core::Qty{0};
        return it->second.depth();
    }
    const auto it = asks_.find(price);
    if (it == asks_.end()) return core::Qty{0};
    return it->second.depth();
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

} // namespace orderbook::book
