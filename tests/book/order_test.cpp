#include <chrono>

#include <gtest/gtest.h>

#include "orderbook/book/order.hpp"

using namespace orderbook::core;
using orderbook::book::Order;

TEST(Order, HoldsItsFields) {
    const Timestamp arrival{std::chrono::nanoseconds{98765}};
    const Order test_order{
        OrderId{1}, Side::Buy, Price{12345}, Qty{100}, arrival
    };
    EXPECT_EQ(test_order.id, OrderId{1});
    EXPECT_EQ(test_order.side, Side::Buy);
    EXPECT_EQ(test_order.price, Price{12345});
    EXPECT_EQ(test_order.qty, Qty{100});
    EXPECT_EQ(test_order.arrival_time, arrival);
}
