#include <optional>

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

    auto& price_queue = (side == core::Side::Buy) ? bids_[price] : asks_[price];
    auto it = price_queue.insert(price_queue.end(), Order{id, side, price, qty, arrival_time});

    orders_by_id_.emplace(id, it);

    return AddResult::Accepted;
}

std::optional<core::Price> OrderBook::best_bid() const {
    if (bids_.empty()) return std::nullopt;
    return bids_.begin()->first;
}

std::optional<core::Price> OrderBook::best_ask() const {
    if (asks_.empty()) return std::nullopt;
    return asks_.begin()->first;
}

} // namespace orderbook::book
