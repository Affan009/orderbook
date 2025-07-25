#pragma once

#include "Includes.h"

enum class OrderType {
    GoodTillCancel,
    FillAndKill,
    FillOrKill,
    GoodForDay,
    Market
};

enum class Side {
    Buy,
    Sell
};

using Price = std::int32_t;
using Quantity = std::uint32_t;
using OrderId = std::uint64_t;
using OrderIds = std::vector<OrderId>;

struct Constants {
    static const Price InvalidPrice = std::numeric_limits<Price>::quiet_NaN();
};
