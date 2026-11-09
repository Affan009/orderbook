# Book events

## What the book reports

* After every call that changes it, the book reports what changed as events, sent to one listener. The caller still gets the call's result enum (for example `CancelResult::Cancelled`); the listener gets the facts of what changed.
* Events describe changes to the book, not the calls that caused them: a modify that loses its place is reported as a removal and then an addition in this exact order.
* Everything a listener needs is in the event, so it never has to query the book again.

## The four events

| Event | Sent when | Fields |
|---|---|---|
| `OrderAdded` | an order joins the book: `add_order`, and the second half of a modify that loses its place or of a replace | `seq, id, side, price, qty, last` |
| `OrderReduced` | an order's quantity goes down and it keeps its place: a partial `reduce_order`, or a modify to a smaller quantity at the same price | `seq, id, side, price, qty, by, last`: `qty` is the remaining quantity, `by` is the quantity reduced by |
| `OrderRemoved` | an order leaves the book: `cancel_order`, a `reduce_order` that takes all of it, and the first half of a modify that loses its place or of a replace | `seq, id, side, price, qty, reason, last`: `qty` is the quantity that left the book (what was still resting) |
| `OrderRejected` | at every place and inside every operation, where validity is violated | `seq, operation, id, reason, last` |

* `RemoveReason` (`Cancelled`, `Modified`, `Reduced`, `Replaced`) exists so that a listener facing a client, such as an exchange gateway, can tell a cancel from a modify, instead of telling the client "your order was cancelled" after a modify.
* `OrderRejected` has no side or price because the rejected call's id may not be in the book at all (for example, cancelling an unknown id).
* Its `reason` is a `std::variant` of the five result enums (`AddResult`, `CancelResult`, `ModifyResult`, `ReduceResult` and `ReplaceResult`), holding the exact value the rejected call returned, so no list of reasons is repeated.
* `BookEvent` is a `std::variant` of the four events, for code that forwards a common event type object.

## Sequence numbers and the end of a call

* `seq` is a `SeqNum`, a strong type from `core`, so it doesn't mix up with other integral types or strong types (like `OrderId` or `Price`). It starts at `1` and grows by one for every event, including rejections, so a listener can check for lost events.
* `last` is true only on the final event emitted by an operation. A reader across a queue knows the book is consistent only after the message with `last = true` is received (the same idea as Databento's `F_LAST`).
* Today only two events are sent with `last = false`: `OrderRemoved` with reason `Modified` inside `modify_order` and `OrderRemoved` with reason `Replaced` inside `replace_order`.

## Rules

* Events are sent after the change is complete, so a listener that reads the book sees the new state.
* A rejected call sends exactly one `OrderRejected` and the book is left unchanged.
* A listener must not call a function that changes the book from inside a callback, because the book may be in the middle of its own operation (for example, half-way through matching against a queue). Debug builds catch it with an `assert` on `in_callback_`; queries are allowed.
* Listener functions are `noexcept`: a listener that threw halfway through a call would leave the events sent so far disagreeing with the book (a removal reported with no matching addition).

## The listener

* `OrderBookListener` has four virtual functions with empty default bodies, so a listener overrides only the events it needs.
* The book takes its listener in the constructor and keeps a pointer that is never null: `OrderBook()` uses a shared, stateless `NullListener`, so the book never checks for a missing listener.
* Each book has exactly one listener, for its whole life. Fan-out to several consumers is the listener's job (or happens after a queue, on another thread), not the book's.
* Calls go through a virtual function, which costs one indirect call per event. `final` on a concrete listener only helps code that holds that concrete type; the book holds an `OrderBookListener*`, so its calls stay virtual.

## How the book sends events

* Public operations change storage only through private helpers (`insert_resting`, `remove_resting`, `reduce_resting`), which never send events. Each public operation sends only its own events after its change, so every event carries the right reason and only the final one has `last = true`.
* `send` hands one event to the listener with `in_callback_` set around the call. `reject` builds an `OrderRejected`, sends it and returns the same reason, so every refusal is one line (`return reject(...)`) that cannot forget the sequence number or `last`.

## Later

* With threads (exchange project), a listener copies each event into a lock-free queue and returns, and the reader thread takes events out and publishes them. `BookEvent` is trivially copyable and fits one 64-byte cache line, which suits a queue slot.
* Logging is a listener too: it hands each event to an asynchronous logger, whose own thread does the slow formatting and file writing.
* Timestamps on events, and trade events, will be added with matching.

## Alternatives considered

* One struct with a kind field: simpler to store, but every event would carry fields that mean nothing for it, and nothing stops code from reading them.
* A template parameter for the listener: no virtual call, but the book would move into headers and change type with every listener. It can be revisited if measurement shows the virtual call matters.
* A list of `std::function` callbacks: more flexible, but slower, and fan-out belongs in a listener.
* Delivering a call's events together at the end: needs a buffer per call, while `last` gives the same boundary for free. This works because the book never changes something and then undoes it.

## References

- Ghosh, S. (2023). *Building Low Latency Applications with C++*. Packt. https://github.com/PacktPublishing/Building-Low-Latency-Applications-with-CPP
- Databento. *Constructing a limit order book*. https://databento.com/docs/examples/order-book/limit-order-book
- intrepidkarthi. *orderbook*. https://github.com/intrepidkarthi/orderbook
- Kotlarska, I. (2024). *Optimising Compiler Performance: A Case for Devirtualisation*. The HRT Beat. https://www.hudsonrivertrading.com/hrtbeat/optimising-compiler-performance-a-case-for-devirtualisation/
