#pragma once

#include <cassert>
#include <list>

#include "orderbook/book/order.hpp"
#include "orderbook/core/types.hpp"

namespace orderbook::book {

// A price level that maintains the order queue and the
// total quantity at that level (depth).
// Depth must always equal the sum of its orders' quantities.
// A price level must have orders of the same side and price only.
class Level {
public:
    using iterator = std::list<Order>::iterator;

    iterator add(Order order) {
        assert(
            (queue_.empty() ||
             (order.side == queue_.front().side && order.price == queue_.front().price)) &&
            "Orders in a price level must have the same side and price"
        );
        depth_ += order.qty;
        return queue_.insert(queue_.end(), order);
    }

    core::Qty depth() const { return depth_; }
    bool empty() const { return queue_.empty(); }

private:
    std::list<Order> queue_;
    core::Qty depth_{0};
};

} // namespace orderbook::book
