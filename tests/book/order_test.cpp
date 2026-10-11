#include <chrono>

#include <gtest/gtest.h>

#include "orderbook/book/order.hpp"

using namespace std::chrono_literals;
using namespace orderbook::core;
using orderbook::book::Order;

TEST(Order, HoldsItsFields) {
    const Timestamp arrival{98765ns};
    const Order order{
        OrderId{1}, Side::Buy, Price{12345}, Qty{100}, arrival
    };
    EXPECT_EQ(order.id, OrderId{1});
    EXPECT_EQ(order.side, Side::Buy);
    EXPECT_EQ(order.price, Price{12345});
    EXPECT_EQ(order.qty, Qty{100});
    EXPECT_EQ(order.arrival_time, arrival);
}
