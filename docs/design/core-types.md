# Core types

## An order and its contents

* An order (limit) is a request to buy or sell some quantity of an instrument at a price equal to or better than a quoted price.
* The price and quantity are always multiples of a tick size and lot size which specify their precision. They are also called resolution parameters of the orderbook.
*  Five pieces of information are tied to each order:

| Piece | Example | Underlying Type | Significance |
|---|---|---|---|
| Side | buy | A scoped enum | Side the order rests in and what it can trade with |
| Price | $187.43 | `int64_t` value (multiple of the tick size) | Quoted price for matching and priority determination |
| Quantity | 100 shares | `int64_t` value (multiple of the lot size)| Amount to be filled |
| Order ID | 42 | `int64_t` value | An identifier for cancelling or modifying orders |
| Arrival time/Timestamp | 09:30:00.000123 | A type from `<chrono>` | A timestamp used for priority determination on same price |

* The instrument symbol itself (say AAPL) would be stored on the orderbook level, not on the order level. It is because an orderbook would contain orders for a single instrument.
* The identity of the trader who sent the order is not relevant to the functioning of the orderbook, so it isn't stored either.

## Price as an integer type

* Price will be stored as the whole number count of the ticks in an `int64_t`. Ex: 186.53 --> 18653, for a tick size of 0.01.
* This is because it lets us avoid floating point imprecision.

## Strong Types

* As many core types are to share the same underlying, they can be passed around interchangeably by mistake. Ex: consider function `add_order(price, qty)`, it would work just as well for a call with interchanged arguments: `add_order(qty, price)` [Wrong].
* We introduce a wrapper over the underlying integer type to differentiate one core type from another. This is easily done by a template, with one tag typename argument per core type.
```cpp
template <typename Tag> // No use inside
class StrongType {
    ...
};

using Price = StrongType<struct PriceTag>;
...
```
* Templates are helpful as they prevent code repetition. For instance, `Price` and `OrderId` are the same except for the tag.
* **NOTE**: `Qty` is a separate class, so as to accommodate for an assert that ensures quantity can never be negative.

## Side as a scoped enum

* The order side is an `enum class` containing only `{ Buy, Sell }`.
* It provides safety by disallowing silent conversion to ints, and of ints to Side.

## Arrival time as a chrono timestamp

* Arrival time is a "Timestamp", which is an alias for the moment on the system clock measured in nanoseconds.
* `std::chrono` is used because it already separates between time points and durations as strong types.
* A moment - moment gives a duration, a moment + duration gives a moment, but a moment + moment is meaningless.
* It is measured in nanoseconds so that timestamps are finer and price-time priority can work effectively with less collisions.
* NOTE: The timestamp will be provided to the orderbook (by the exchange or from outside, not by the trader ever!), so that it doesn't read the system clock and it is easier to replay events (which makes the test deterministic, important for random testing later).
