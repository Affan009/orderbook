#include <chrono>
#include <type_traits>
#include <gtest/gtest.h>

#include "orderbook/core/types.hpp"

using namespace std::chrono;
using namespace orderbook::core;

template <typename T>
concept Addable = requires(T a, T b) { a + b; };

static_assert(Addable<int>, "sanity check: ints can be added");
static_assert(!Addable<Price>, "Price + Price must not compile");
static_assert(!Addable<OrderId>, "OrderId + OrderId must not compile");

static_assert(!std::is_convertible_v<std::int64_t, Price>, "an int64_t must not silently become a Price");
static_assert(!std::is_convertible_v<Price, std::int64_t>, "a Price must not silently become an int64_t");

static_assert(!std::is_convertible_v<Qty, Price>, "a Qty must not silently become a Price");
static_assert(!std::is_convertible_v<Price, Qty>, "a Price must not silently become a Qty");

static_assert(!std::is_convertible_v<OrderId, Price>, "an OrderId must not silently become a Price");
static_assert(!std::is_convertible_v<Price, OrderId>, "a Price must not silently become an OrderId");

static_assert(!std::is_convertible_v<int, Side>, "an int must not silently become a Side");
static_assert(!std::is_convertible_v<Side, int>, "a Side must not silently become an int");

static_assert(std::is_same_v<Timestamp::duration, std::chrono::nanoseconds>, "Duration must be in units of nanoseconds");

TEST(Price, StoresTicksExactly) {
    const Price test_price{12345};
    EXPECT_EQ(test_price.value(), 12345);
}

TEST(Price, EqualTickCountsAreEqual) {
    const Price test_price_1{12345};
    const Price test_price_2{12345};
    EXPECT_EQ(test_price_1, test_price_2);
}

TEST(Price, FewerTicksIsLess) {
    const Price test_price_1{12344};
    const Price test_price_2{12345};
    EXPECT_LT(test_price_1, test_price_2);
}

TEST(Qty, StoresLotsExactly) {
    const Qty test_quantity{123};
    EXPECT_EQ(test_quantity.value(), 123);
}

TEST(Qty, ZeroQuantityAllowed) {
    const Qty test_quantity{0};
    EXPECT_EQ(test_quantity.value(), 0);
}

TEST(Qty, MoreLotsIsLarger) {
    const Qty test_quantity_1{123};
    const Qty test_quantity_2{122};
    EXPECT_GT(test_quantity_1, test_quantity_2);
}

TEST(Qty, NegativeQuantityDies) {
    EXPECT_DEBUG_DEATH(Qty{-123}, "Quantity must always be non-negative");
}

TEST(OrderId, StoresIdExactly) {
    const OrderId test_id{1};
    EXPECT_EQ(test_id.value(), 1);
}

TEST(OrderId, EqualIdsAreEqual) {
    const OrderId test_id_1{1};
    const OrderId test_id_2{1};
    EXPECT_EQ(test_id_1, test_id_2);
}

TEST(Side, SidesAreDistinct) {
    EXPECT_NE(Side::Buy, Side::Sell);
}

TEST(Timestamp, LaterTimeIsGreater) {
    const Timestamp test_time_1{nanoseconds{1}};
    const Timestamp test_time_2 = test_time_1 + nanoseconds{1};
    EXPECT_LT(test_time_1, test_time_2);
}
