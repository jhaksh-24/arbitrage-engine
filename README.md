# CELAE — Cross-Exchange Latency Arbitrage Engine

A high-performance, low-latency cross-exchange arbitrage engine built in **C++20**. Detects and simulates exploitation of price discrepancies between Binance and Bybit at microsecond-level speeds using WebSocket market data feeds, SIMD-accelerated JSON parsing, and a cache-friendly chunked-array order book.

> **Status:** Phase 4 complete — Arbitrage Strategy running against live dual-exchange feeds

---

## Architecture

```
┌─────────────────────────────────────────────────────┐
│                  STRATEGY LAYER                      │
│  ArbitrageStrategy: cross-exchange spread detection, │
│  fee-adjusted profit calculation, trade simulation   │
├──────────────────────┬──────────────────────────────┤
│   BINANCE            │       BYBIT                   │
│  ┌──────────────┐    │    ┌──────────────┐          │
│  │ BinanceFeed  │    │    │  BybitFeed   │          │
│  │ (WebSocket)  │    │    │ (WebSocket)  │          │
│  └──────┬───────┘    │    └──────┬───────┘          │
│         │            │           │                   │
│  ┌──────▼───────┐    │    ┌──────▼───────┐          │
│  │  OrderBook   │    │    │  OrderBook   │          │
│  │ <32,16>      │    │    │ <32,16>      │          │
│  └──────────────┘    │    └──────────────┘          │
├──────────────────────┴──────────────────────────────┤
│              CORE INFRASTRUCTURE                     │
│  Strong types, fixed-point arithmetic, lock-free     │
│  ring buffer, memory pool, ScopedTimer, fast logger  │
└─────────────────────────────────────────────────────┘
```

---

## Directory Structure

```
CELAE/
├── CMakeLists.txt                 # Build configuration (CMake 3.20+, C++20)
├── apps/
│   ├── main.cpp                   # Entry point
│   └── live_feed_test.cpp         # Dual-exchange live test (Binance + Bybit + Strategy)
├── include/arb/
│   ├── core/
│   │   ├── types.hpp              # Fundamental types: Price, Quantity, Timestamp, OrderId
│   │   ├── price_level.hpp        # L2 price level (price + quantity aggregate)
│   │   ├── order.hpp              # Order struct (aggregate for execution layer)
│   │   └── order_book.hpp         # Chunked-array order book (BookSide, OrderBook)
│   ├── net/
│   │   ├── binance_feed.hpp       # Binance L2 depth feed handler (IXWebSocket + simdjson)
│   │   └── bybit_feed.hpp         # Bybit V5 Linear Perps feed handler
│   ├── strategy/
│   │   └── arbitrage_strategy.hpp # Cross-exchange arbitrage detection & trade simulation
│   └── utils/
│       ├── clock.hpp              # High-resolution timing (steady_clock, ScopedTimer)
│       ├── logger.hpp             # Low-latency synchronous logger (printf-based)
│       ├── ring_buffer.hpp        # Lock-free SPSC ring buffer for inter-thread comms
│       └── memory_pool.hpp        # Pre-allocated object pool (zero-alloc hot path)
├── src/
│   ├── net/
│   │   ├── binance_feed.cpp       # Binance WebSocket feed handler implementation
│   │   └── bybit_feed.cpp         # Bybit WebSocket feed handler implementation
│   └── strategy/
│       └── arbitrage_strategy.cpp # Arbitrage strategy implementation
├── tests/
│   └── test_order_book.cpp        # Unit tests for OrderBook (Google Test)
├── bench/
│   └── bench_order_book.cpp       # Latency benchmarks for OrderBook (Google Benchmark)
└── docs/                          # Architecture & tuning docs (future)
```

---

## Key Design Decisions

### Fixed-Point Arithmetic
All monetary values use `int64_t` with a scale factor of `10^8`, not `double`. Floating-point arithmetic is non-deterministic (`0.1 + 0.2 ≠ 0.3`) — unacceptable when real money is at stake.

### Strong Types
`Price`, `Quantity`, `Timestamp`, `OrderId`, and `Duration` are all distinct types via a `StrongType<T, Tag>` wrapper. The compiler prevents accidentally passing a quantity where a price is expected.

### SIMD-Accelerated JSON Parsing
Both feed handlers use [simdjson](https://github.com/simdjson/simdjson) `ondemand` for zero-copy, hardware-accelerated JSON parsing. The Bybit parser iterates fields directly to handle non-deterministic key ordering in the JSON payload — a subtle but critical detail for `simdjson`'s forward-only iterator model.

### Lock-Free Communication
Inter-thread communication uses a SPSC (Single-Producer, Single-Consumer) ring buffer with:
- Power-of-2 capacity (bitwise AND instead of modulo)
- `alignas(64)` on atomic indices (prevents false sharing across cache lines)
- `memory_order_acquire` / `memory_order_release` for correct ordering without locks

### Cache-Friendly Order Book
The order book is designed for minimal cache misses and zero heap allocation on the hot path:
- Price space is split into fixed-size **chunks** — O(1) chunk lookup, bounded O(ChunkSize) shifts on insert/remove
- Flat arrays instead of pointer-heavy trees
- Pre-allocated memory pool for order storage
- O(1) best bid/ask access

### Low-Latency Logger
The logger uses `printf` instead of `std::cout` to avoid the overhead of C++ stream formatting and flushing. All log lines are prefixed with a high-resolution timestamp for latency analysis.

---

## Live Demo Output

```
[1783626375347] [INFO] BINANCE  | BID:    6333158 | ASK:    6333159
[1783626375348] [INFO] BYBIT    | BID:    6330090 | ASK:    6330100
[1783626375349] [INFO] [ARB DETECTED] Buy Bybit / Sell Binance | Profit Ticks: 3058.00 | Qty: 46317000
[1783626375349] [INFO] -------------------------------------------------
```

With 0% fees (simulating VIP-tier access), the engine detects cross-exchange price discrepancies on every single tick. With realistic 0.05% taker fees, profitable opportunities are rare — which is exactly how real markets work.

---

## Build

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

**Requirements:** CMake 3.20+, C++20 compiler (MSVC 19.29+, GCC 11+, Clang 14+)

**Dependencies** (fetched automatically by CMake):
- [IXWebSocket](https://github.com/machinezone/IXWebSocket) — WebSocket client
- [simdjson](https://github.com/simdjson/simdjson) — SIMD JSON parser
- [Google Test](https://github.com/google/googletest) — Unit testing
- [Google Benchmark](https://github.com/google/benchmark) — Microbenchmarks

---

## Testing

```bash
# From the build directory
cmake --build . --target test_order_book
ctest --output-on-failure
```

Current test coverage (`tests/test_order_book.cpp`):

| Test | Description |
|---|---|
| `EmptyBook` | Fresh book has invalid/zero best bid and ask |
| `SingleBidInsert` | Single bid is placed correctly |
| `SingleAskInsert` | Single ask is placed correctly |
| `MultipleBidsSorting` | Bids are kept in descending price order |
| `MultipleAsksSorting` | Asks are kept in ascending price order |
| `UpdateLevel` | Re-inserting at an existing price updates quantity, not adds |
| `SpreadCalc` | Spread = best ask − best bid |
| `ClearBook` | Clearing resets both sides to empty |
| `CrossChunkInsert` | 40+ levels force a chunk split; best bid/ask remain correct |

---

## Benchmarks

```bash
# From the build directory
cmake --build . --target bench_order_book
./bench/bench_order_book
```

| Benchmark | What it measures | stats on my end |
|---|---|---|
| `BM_OrderBookUpdate` | Throughput of `update()` with scattered prices (100k iterations) | 11-12 ns |
| `BM_OrderBookBestBidRead` | Raw read latency of `best_bid()` (1M iterations) | 0.114-0.156 ns |

---

## Roadmap

- [x] **Phase 1** — Core types, clock, ring buffer, memory pool
- [x] **Phase 2** — High-performance order book + unit tests + benchmarks
- [x] **Phase 3** — Exchange connectivity: Binance WebSocket feed handler
- [x] **Phase 3.5** — Exchange connectivity: Bybit WebSocket feed handler
- [x] **Phase 4** — Arbitrage detection strategy (fee-adjusted, with trade simulation)
- [ ] **Phase 5** — Order execution layer (HMAC-SHA256 signed REST orders)
- [ ] **Phase 6** — Risk management & kill switch
- [ ] **Phase 7** — Integration, optimization, kernel bypass (Linux)

---

## License

MIT

