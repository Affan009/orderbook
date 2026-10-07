#pragma once

#include <stdexcept>
#include <string>
#include <utility>

#include "orderbook/core/types.hpp"

namespace orderbook::core {

// Instrument holds the identity (symbol) and resolution parameters (tick and lot size).
// An orderbook holds orders for exactly one instrument.
class Instrument {
public:
    Instrument(std::string symbol, TickSize tick_size, LotSize lot_size)
        : symbol_{std::move(symbol)}, tick_size_{tick_size}, lot_size_{lot_size} {
        // These values come from configuration, so they are reported as exceptions (not a bug).
        // These checks also run in release builds, unlike asserts.
        if (tick_size_.value() <= 0) {
            throw std::invalid_argument{"tick size must be positive"};
        }
        if (lot_size_.value() <= 0) {
            throw std::invalid_argument{"lot size must be positive"};
        }
    }

    const std::string& symbol() const { return symbol_; }
    TickSize tick_size() const { return tick_size_; }
    LotSize lot_size() const { return lot_size_; }

private:
    std::string symbol_;
    TickSize tick_size_;
    LotSize lot_size_;
};

} // namespace orderbook::core
