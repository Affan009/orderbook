# Core types

## Order

* An order (limit) is a request to buy or sell some quantity of an instrument at a price equal to or better than a quoted price.
* The price and quantity are always multiples of a tick size and lot size which specify their precision. They are also called resolution parameters of the orderbook.
*  Five pieces of information are tied to each order:

| Piece | Example | C++ Types | Significance |
|---|---|---|---|
| Side | buy | A scoped enum | Side the order rests in and what it can trade with |
| Price | $187.43 | `int64_t` value (multiple of the tick size) | Quoted price for matching and priority determination |
| Quantity | 100 shares | `int64_t` value (multiple of the lot size)| Amount to be filled |
| Order ID | 42 | `int64_t` value (a plain old incremented integer) | An identifier for cancelling or modifying orders |
| Arrival time | 09:30:00.000123 | A type from `<chrono>` | A timestamp used for priority determination on same price |

* The instrument symbol itself (say AAPL) would be stored on the orderbook level, not on the order level. It is because an orderbook would contain orders for a single instrument.
* The identity of the trader who sent the order is not relevant to the functioning of the orderbook, so it isn't stored either.
