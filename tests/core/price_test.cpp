#include <gtest/gtest.h>
#include "orderbook/core/price.hpp"

TEST(Spread, positive) {
    EXPECT_EQ(spread(5, 6), 1);
}

TEST(Spread, invalid) {
    EXPECT_LT(spread(6, 5), 0);
}
