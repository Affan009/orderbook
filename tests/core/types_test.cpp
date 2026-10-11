#include <chrono>
#include <type_traits>
#include <gtest/gtest.h>

#include "orderbook/core/types.hpp"

using namespace std::chrono_literals;
using namespace orderbook::core;

template <typename T>
concept Addable = requires(T a, T b) { a + b; };

template<typename T>
concept Subtractable = requires(T a, T b) { a - b; };

static_assert(Addable<int>, "sanity check: ints can be added");
static_assert(!Addable<Price>, "Price + Price must not compile");
static_assert(!Addable<OrderId>, "OrderId + OrderId must not compile");
static_assert(Addable<Qty>, "Qty + Qty must compile");
static_assert(Subtractable<Qty>, "Qty - Qty must compile");

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
    const Price price{12345};
    EXPECT_EQ(price.value(), 12345);
}

TEST(Price, EqualTickCountsAreEqual) {
    const Price price_1{12345};
    const Price price_2{12345};
    EXPECT_EQ(price_1, price_2);
}

TEST(Price, FewerTicksIsLess) {
    const Price lower_price{12344};
    const Price higher_price{12345};
    EXPECT_LT(lower_price, higher_price);
}

TEST(Qty, StoresLotsExactly) {
    const Qty quantity{123};
    EXPECT_EQ(quantity.value(), 123);
}

TEST(Qty, ZeroQuantityAllowed) {
    const Qty quantity{0};
    EXPECT_EQ(quantity.value(), 0);
}

TEST(Qty, MoreLotsIsLarger) {
    const Qty larger_quantity{123};
    const Qty smaller_quantity{122};
    EXPECT_GT(larger_quantity, smaller_quantity);
}

TEST(Qty, NegativeQuantityDies) {
    EXPECT_DEBUG_DEATH(Qty{-123}, "Quantity must always be non-negative");
}

TEST(Qty, TwoQuantitiesAdd) {
    const Qty quantity_1{100};
    const Qty quantity_2{99};
    const Qty result = quantity_1 + quantity_2;
    EXPECT_EQ(result, Qty{199});
}

TEST(Qty, TwoQuantitiesSubtract) {
    const Qty quantity_1{100};
    const Qty quantity_2{99};
    const Qty result = quantity_1 - quantity_2;
    EXPECT_EQ(result, Qty{1});
}

TEST(Qty, SubtractionMustGiveNonNegativeResult) {
    const Qty quantity_1{100};
    const Qty quantity_2{101};
    EXPECT_DEBUG_DEATH(quantity_1 - quantity_2, "Quantity must always be non-negative");
}

TEST(OrderId, StoresIdExactly) {
    const OrderId id{1};
    EXPECT_EQ(id.value(), 1);
}

TEST(OrderId, EqualIdsAreEqual) {
    const OrderId id_1{1};
    const OrderId id_2{1};
    EXPECT_EQ(id_1, id_2);
}

TEST(Side, SidesAreDistinct) {
    EXPECT_NE(Side::Buy, Side::Sell);
}

TEST(Timestamp, LaterTimeIsGreater) {
    const Timestamp time_1{1ns};
    const Timestamp time_2 = time_1 + 1ns;
    EXPECT_LT(time_1, time_2);
}
