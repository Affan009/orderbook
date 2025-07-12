# orderbook

# 🧾 Order Book Matching Engine (C++)

This is a high-performance **limit order book** matching engine implemented in modern **C++20**. It supports market, limit, fill-or-kill, and good-for-day orders with multithreaded pruning logic and price-time priority matching.

---

## Features

-  **Bid/Ask Side Order Books** with price-level aggregation  
-  **Fast Matching Engine** with support for:
  - Limit Orders
  - Market Orders (converted to GTC)
  - Fill-or-Kill / Fill-and-Kill logic
  - Good-for-Day auto-expiry
-  **Matching Logic** based on price-time priority
-  **Background Thread** for pruning GFD orders after market hours
-  **Thread-safe access** using `std::mutex` and `std::scoped_lock`
-  Level-wise market data retrieval via `GetOrderInfos()`

---

