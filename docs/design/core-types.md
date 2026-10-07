# Core types

## An order and its contents

* An order (limit) is a request to buy or sell some quantity of an instrument at a price equal to or better than a quoted price.
* The price and quantity are always multiples of a tick size and lot size which specify their precision. They are also called resolution parameters of the orderbook.
*  Five pieces of information are tied to each order:

| Piece | Example | Underlying Type | Significance |
|---|---|---|---|
| Side | buy | A scoped enum | Side the order rests in and what it can trade with |
| Price | $187.43 | `int64_t` value (count of the ticks) | Quoted price for matching and priority determination |
| Quantity | 100 shares | `int64_t` value (count of the lot steps)| Amount to be filled |
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
* **NOTE**: The timestamp will be provided to the orderbook (by the exchange or from outside, not by the trader ever!), so that it doesn't read the system clock and it is easier to replay events (which makes the test deterministic, important for random testing later).

## Instrument

* An Instrument holds the symbol, tick size and lot size of an instrument/asset.
* Tick and lot sizes are stored as whole numbers of billionths (`FIXED_POINT_SCALE = 1'000'000'000`)
*  Tick and lot sizes must be strictly positive. Instruments come with configuration, so a bad value throws (checked in each build).
*  **NOTES**: 
1. The decision for storage of tick and lot sizes was modelled from CME's binary price format (`PRICENULL9`: an `int64_t` mantissa with a fixed `-9` exponent).
2. `Qty` counts steps, not billionths, so with 1-share lot, 100 shares is `Qty{ 100 }` and with 0.001 BTC step, 0.005 is `Qty{ 5 }`.
3. Instrument is not used by the book, the book only works on ticks and lots. It is used when loading and/or displaying data.

## References

- Gould, M. D., Porter, M. A., Williams, S., McDonald, M., Fenn, D. J., & Howison, S. D. (2013).
  *Limit order books*. Quantitative Finance, 13(11). https://arxiv.org/abs/1012.0349
  — "resolution parameters" (tick size and lot size), section III.G.
- Boccara, J. (2016). *Strong types for strong interfaces*. Fluent C++.
  https://www.fluentcpp.com/2016/12/08/strong-types-for-strong-interfaces/
  — the tagged wrapper pattern behind `StrongType`.
- cppreference: `std::chrono::time_point` and `std::chrono::duration`.
  https://en.cppreference.com/w/cpp/chrono/time_point ·
  https://en.cppreference.com/w/cpp/chrono/duration
  — moment and length-of-time types used for `Timestamp`.
- CME Group. *MDP 3.0 – CME Globex Pricing*.
  https://cmegroupclientsite.atlassian.net/wiki/spaces/EPICSANDBOX/pages/457225869/MDP+3.0+-+CME+Globex+Pricing
  — tick size as the minimum price fluctuation (tag 969, MinPriceIncrement).
- CME Group. *MDP 3.0 – SBE Decoding Example*.
  https://cmegroupclientsite.atlassian.net/wiki/display/EPICSANDBOX/MDP+3.0+-+SBE+Decoding+Example
  — PRICE9 / PRICENULL9: an int64 mantissa with a constant exponent of -9.
