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
