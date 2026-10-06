#include <chrono>
#include <gtest/gtest.h>
#include <type_traits>
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
    Price testPrice{12345};
    EXPECT_EQ(testPrice.value(), 12345);
}

TEST(Price, EqualTickCountsAreEqual) {
    Price testPrice1{12345};
    Price testPrice2{12345};
    EXPECT_EQ(testPrice1, testPrice2);
}

TEST(Price, FewerTicksIsLess) {
    Price testPrice1{12344};
    Price testPrice2{12345};
    EXPECT_LT(testPrice1, testPrice2);
}

TEST(Qty, StoresLotsExactly) {
    Qty testQuantity{123};
    EXPECT_EQ(testQuantity.value(), 123);
}

TEST(Qty, ZeroQuantityAllowed) {
    Qty testQuantity{0};
    EXPECT_EQ(testQuantity.value(), 0);
}

TEST(Qty, MoreLotsIsLarger) {
    Qty testQuantity1{123};
    Qty testQuantity2{122};
    EXPECT_GT(testQuantity1, testQuantity2);
}

TEST(Qty, NegativeQuantityDies) {
    EXPECT_DEBUG_DEATH(Qty{-123}, "Quantity must always be non-negative");
}

TEST(OrderId, StoresIdExactly) {
    OrderId testId{1};
    EXPECT_EQ(testId.value(), 1);
}

TEST(OrderId, EqualIdsAreEqual) {
    OrderId testId1{1};
    OrderId testId2{1};
    EXPECT_EQ(testId1, testId2);
}

TEST(Side, SidesAreDistinct) {
    EXPECT_NE(Side::Buy, Side::Sell);
}

TEST(Timestamp, LaterTimeIsGreater) {
    Timestamp testTime1{nanoseconds{1}};
    Timestamp testTime2 = testTime1 + nanoseconds{1};
    EXPECT_LT(testTime1, testTime2);
}
