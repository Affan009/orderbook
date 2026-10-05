#include <gtest/gtest.h>
#include "orderbook/core/types.hpp"

using namespace orderbook::core;

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
