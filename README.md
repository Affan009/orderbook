# orderbook

A C++ implementation of a limit order book.

## Background

* A limit order book is a structure that holds the bids and asks of an instrument sorted by price and time.
* Bids are the prices quoted for the resting buy orders of the instrument, while asks are the prices quoted for the resting sell orders of the instrument.
* A matching engine matches buy and sell orders to execute a trade. It does so in accordance with price-time priority, which means best price orders match first, and on same price those that were first are matched.
* I am building this to gain experience in low latency C++ development, especially in the context of high frequency trading.

## Build

* Requirements: C++20, GCC 11.4.0, CMake >= 3.22, Ninja 1.10.1, libgtest-dev 1.11

* Two build presets are available: `release` which enables compiler optimizations and `asan` compiles without optimizations and enables sanitizers: AddressSanitizer and UndefinedBehaviorSanitizer for debugging.

```bash
# configure preset
cmake --preset <preset>

# build preset
cmake --build --preset <preset>

# run tests for the preset
ctest --preset <preset>
```

* Everything was tested on Ubuntu 22.04 under WSL2.

## Usage

* Libraries: `orderbook::core` (value types) and `orderbook::book` (the order book, links `core`).
* Linking: add this repository with `add_subdirectory(orderbook)`, then `target_link_libraries(<target> PRIVATE orderbook::book)`.
* Units: prices are whole numbers of ticks, quantities whole numbers of lots.

## Documentation

* `docs/design/core.md`: value types.
* `docs/design/book.md`: storage, operations, invariant checker.
* `docs/design/events.md`: events and the listener.

## Project Status

- [x] Core types: `Price`, `Qty`, `OrderId`, `Side`, `Timestamp`, `SeqNum`, `Instrument`
- [x] Resting orders for one instrument: add, cancel, modify, reduce, replace
- [x] Price-time priority, with modify keeping or losing queue position
- [x] Queries: best bid/ask, depth at a price, order lookup, quantity ahead in the queue
- [x] Text view of the book
- [x] Invariant checker
- [x] Events for every change and rejection, sent to a listener
- [ ] Matching (orders that cross the spread)
- [ ] Market, IOC and FOK orders
- [ ] Benchmarks

## Contributing

* Issues accepted, pull requests not accepted.
