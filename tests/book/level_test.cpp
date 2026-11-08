#include <chrono>
#include <iterator>

#include <gtest/gtest.h>

#include "orderbook/book/level.hpp"
#include "orderbook/core/types.hpp"

using namespace std::chrono_literals;
using namespace orderbook::core;
using orderbook::book::Order;
using orderbook::book::Level;

TEST(Level, EmptyLevelHasZeroDepth) {
    const Level price_level;
    EXPECT_TRUE(price_level.empty());
    EXPECT_EQ(price_level.depth(), Qty{0});
}

TEST(Level, DepthIsTotalQuantity) {
    Level price_level;
    price_level.add(Order{OrderId{1}, Side::Buy, Price{12345}, Qty{100}, Timestamp{1ns}});
    price_level.add(Order{OrderId{2}, Side::Buy, Price{12345}, Qty{200}, Timestamp{2ns}});
    EXPECT_EQ(price_level.depth(), Qty{300});
}

TEST(Level, OrderCountIsNumberOfRestingOrders) {
    Level price_level;
    EXPECT_EQ(price_level.order_count(), 0U);
    price_level.add(Order{OrderId{1}, Side::Buy, Price{12345}, Qty{100}, Timestamp{1ns}});
    price_level.add(Order{OrderId{2}, Side::Buy, Price{12345}, Qty{200}, Timestamp{2ns}});
    EXPECT_EQ(price_level.order_count(), 2U);
}

TEST(Level, AddReturnsIteratorToAddedOrder) {
    Level price_level;
    const auto order_it = price_level.add(
        Order{OrderId{1}, Side::Buy, Price{12345}, Qty{100}, Timestamp{1ns}}
    );
    EXPECT_EQ(order_it->id, OrderId{1});
}

TEST(Level, AddKeepsArrivalOrder) {
    Level price_level;
    const auto first_order_it = price_level.add(
        Order{OrderId{1}, Side::Buy, Price{12345}, Qty{100}, Timestamp{1ns}}
    );
    const auto second_order_it = price_level.add(
        Order{OrderId{2}, Side::Buy, Price{12345}, Qty{200}, Timestamp{2ns}}
    );

    // Second order sits next to the first order.
    // Arrival order is kept (time priority).
    EXPECT_EQ(std::next(first_order_it), second_order_it);
}

TEST(Level, RemoveLowersDepthAndOrderCount) {
    Level price_level;
    const auto first_order_it = price_level.add(
        Order{OrderId{1}, Side::Buy, Price{12345}, Qty{100}, Timestamp{1ns}}
    );
    price_level.add(Order{OrderId{2}, Side::Buy, Price{12345}, Qty{200}, Timestamp{2ns}});

    price_level.remove(first_order_it);

    EXPECT_EQ(price_level.depth(), Qty{200});
    EXPECT_EQ(price_level.order_count(), 1U);
}

TEST(Level, RemovingMiddleOrderKeepsOthersInArrivalOrder) {
    Level price_level;
    const auto first_order_it = price_level.add(
        Order{OrderId{1}, Side::Buy, Price{12345}, Qty{100}, Timestamp{1ns}}
    );
    const auto second_order_it = price_level.add(
        Order{OrderId{2}, Side::Buy, Price{12345}, Qty{200}, Timestamp{2ns}}
    );
    const auto third_order_it = price_level.add(
        Order{OrderId{3}, Side::Buy, Price{12345}, Qty{300}, Timestamp{3ns}}
    );

    price_level.remove(second_order_it);

    // The other iterators stay valid, and the third order now follows the first.
    EXPECT_EQ(std::next(first_order_it), third_order_it);
    EXPECT_EQ(first_order_it->id, OrderId{1});
    EXPECT_EQ(third_order_it->id, OrderId{3});
}

TEST(Level, RemovingOnlyOrderLeavesEmptyLevel) {
    Level price_level;
    const auto order_it = price_level.add(
        Order{OrderId{1}, Side::Buy, Price{12345}, Qty{100}, Timestamp{1ns}}
    );

    price_level.remove(order_it);

    EXPECT_TRUE(price_level.empty());
    EXPECT_EQ(price_level.depth(), Qty{0});
}

TEST(Level, ReduceLowersOrderQtyAndDepthInPlace) {
    Level price_level;
    const auto first_order_it = price_level.add(
        Order{OrderId{1}, Side::Buy, Price{12345}, Qty{100}, Timestamp{1ns}}
    );
    const auto second_order_it = price_level.add(
        Order{OrderId{2}, Side::Buy, Price{12345}, Qty{200}, Timestamp{2ns}}
    );
    price_level.add(Order{OrderId{3}, Side::Buy, Price{12345}, Qty{300}, Timestamp{3ns}});

    price_level.reduce(second_order_it, Qty{50});

    EXPECT_EQ(second_order_it->qty, Qty{150});
    EXPECT_EQ(price_level.depth(), Qty{550});
    EXPECT_EQ(std::next(first_order_it), second_order_it);  // still in the same place
}

TEST(Level, ReduceTakingWholeOrderDies) {
    // Removing a whole order is remove()'s job, so reduce() requires something to remain.
    Level price_level;
    const auto order_it = price_level.add(
        Order{OrderId{1}, Side::Buy, Price{12345}, Qty{100}, Timestamp{1ns}}
    );
    EXPECT_DEBUG_DEATH(
        price_level.reduce(order_it, Qty{100}),
        "Reduction must leave a positive quantity"
    );
}

TEST(Level, AddDiesOnDifferentSide) {
    Level price_level;
    price_level.add(Order{OrderId{1}, Side::Buy, Price{12345}, Qty{100}, Timestamp{1ns}});
    EXPECT_DEBUG_DEATH(
        price_level.add(Order{OrderId{2}, Side::Sell, Price{12345}, Qty{200}, Timestamp{2ns}}),
        "Orders in a price level must have the same side and price"
    );
}

TEST(Level, AddDiesOnDifferentPrice) {
    Level price_level;
    price_level.add(Order{OrderId{1}, Side::Buy, Price{12345}, Qty{100}, Timestamp{1ns}});
    EXPECT_DEBUG_DEATH(
        price_level.add(Order{OrderId{2}, Side::Buy, Price{12346}, Qty{200}, Timestamp{2ns}}),
        "Orders in a price level must have the same side and price"
    );
}
