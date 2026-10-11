#include <stdexcept>
#include <gtest/gtest.h>

#include "orderbook/core/instrument.hpp"

using namespace orderbook::core;

TEST(Instrument, StoresItsFields) {
    const Instrument apple{"AAPL", TickSize{fixed_point_scale / 100}, LotSize{fixed_point_scale}};
    EXPECT_EQ(apple.symbol(), "AAPL");
    EXPECT_EQ(apple.tick_size(), TickSize{fixed_point_scale / 100});
    EXPECT_EQ(apple.lot_size(), LotSize{fixed_point_scale});
}

TEST(Instrument, ThrowsOnZeroTickSize) {
    EXPECT_THROW(Instrument("AAPL", TickSize{0}, LotSize{fixed_point_scale}), std::invalid_argument);
}

TEST(Instrument, ThrowsOnNegativeLotSize) {
    EXPECT_THROW(Instrument("AAPL", TickSize{fixed_point_scale / 100}, LotSize{-fixed_point_scale}), std::invalid_argument);
}
