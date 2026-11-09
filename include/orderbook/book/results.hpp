#pragma once

#include <cstdint>

namespace orderbook::book {

// Tells the result of an order addition/cancellation to the book.
enum class AddResult : std::uint8_t { Accepted, DuplicateOrderId, ZeroQuantity };
enum class CancelResult : std::uint8_t { Cancelled, UnknownOrderId };

// Tells the result of an order modification/reduction/replacement to the book.
enum class ModifyResult : std::uint8_t { Modified, UnknownOrderId, ZeroQuantity };
enum class ReduceResult : std::uint8_t { Reduced, Removed, UnknownOrderId, ZeroQuantity };
enum class ReplaceResult : std::uint8_t {
    Replaced, UnknownOrderId, DuplicateOrderId, ZeroQuantity
};

} // namespace orderbook::book
