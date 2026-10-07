#include <stdexcept>
#include <gtest/gtest.h>

#include "orderbook/core/instrument.hpp"

using namespace orderbook::core;

TEST(Instrument, StoresItsFields) {
    Instrument test_instrument{"AAPL", TickSize{FIXED_POINT_SCALE / 100}, LotSize{FIXED_POINT_SCALE}};
    EXPECT_EQ(test_instrument.symbol(), "AAPL");
    EXPECT_EQ(test_instrument.tick_size(), TickSize{FIXED_POINT_SCALE / 100});
    EXPECT_EQ(test_instrument.lot_size(), LotSize{FIXED_POINT_SCALE});
}

TEST(Instrument, ThrowsOnZeroTickSize) {
    EXPECT_THROW(Instrument("AAPL", TickSize{0}, LotSize{FIXED_POINT_SCALE}), std::invalid_argument);
}

TEST(Instrument, ThrowsOnNegativeLotSize) {
    EXPECT_THROW(Instrument("AAPL", TickSize{FIXED_POINT_SCALE / 100}, LotSize{-FIXED_POINT_SCALE}), std::invalid_argument);
}
