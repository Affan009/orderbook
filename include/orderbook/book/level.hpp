#pragma once

#include <cassert>
#include <cstddef>
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

    void remove(iterator order_it) {
        depth_ -= order_it->qty;
        queue_.erase(order_it);
    }

    void reduce(iterator order_it, core::Qty by) {
        assert((order_it->qty > by) && "Reduction must leave a positive quantity");
        order_it->qty -= by;
        depth_ -= by;
    }

    core::Qty qty_ahead(iterator order_it) const {
        core::Qty ahead{0};
        for (auto it = queue_.begin(); it != order_it; ++it) {
            ahead += it->qty;
        }
        return ahead;
    }

    core::Qty depth() const { return depth_; }
    std::size_t order_count() const { return queue_.size(); }
    bool empty() const { return queue_.empty(); }

private:
    std::list<Order> queue_;
    core::Qty depth_{0};
};

} // namespace orderbook::book
