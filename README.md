# orderbook

A C++ implementation of a limit order book and a matching engine.

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

* Nothing particular right now, build and run the tests.

## Project Status

* It is currently in the initial steps. Next is defining the core types to be used in the project.

## Contributing

* Issues accepted, pull requests not accepted.
