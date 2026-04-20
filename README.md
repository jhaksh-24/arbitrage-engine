# CELAE — Cross-Exchange Latency Arbitrage Engine

A high-performance, low-latency cross-exchange arbitrage engine built in **C++20**. Designed to detect and exploit price discrepancies across cryptocurrency exchanges at microsecond-level speeds.

> **Status:** Phase 3 — Exchange Connectivity (in progress)

---

## Architecture

```
┌─────────────────────────────────────────────────────┐
│                  STRATEGY LAYER                      │
│  Arbitrage detection, signal generation, position    │
│  management, risk checks                             │
├─────────────────────────────────────────────────────┤
│               ORDER MANAGEMENT SYSTEM                │
│  Order routing, fill tracking, state machine         │
├──────────────────────┬──────────────────────────────┤
│   EXCHANGE A         │       EXCHANGE B              │
│  ┌──────────────┐    │    ┌──────────────┐          │
│  │ Feed Handler │    │    │ Feed Handler │          │
│  └──────────────┘    │    └──────────────┘          │
│  ┌──────────────┐    │    ┌──────────────┐          │
│  │ Order Gateway│    │    │ Order Gateway│          │
│  └──────────────┘    │    └──────────────┘          │
├──────────────────────┴──────────────────────────────┤
│              NETWORK / CONNECTIVITY LAYER            │
│  Kernel bypass, raw sockets, colocation              │
└─────────────────────────────────────────────────────┘
```

---

## Directory Structure

```
CELAE/
├── CMakeLists.txt                 # Build configuration (CMake 3.20+, C++20)
├── apps/
│   └── main.cpp                   # Entry point
├── include/arb/
│   ├── core/
│   │   ├── types.hpp              # Fundamental types: Price, Quantity, Timestamp, OrderId
│   │   ├── price_level.hpp        # L2 price level (price + quantity aggregate)
│   │   ├── order.hpp              # Order struct (aggregate for execution layer)
│   │   └── order_book.hpp         # Chunked-array order book (BookSide, OrderBook)
│   └── utils/
│       ├── clock.hpp              # High-resolution timing (steady_clock, ScopedTimer)
│       ├── ring_buffer.hpp        # Lock-free SPSC ring buffer for inter-thread comms
│       └── memory_pool.hpp        # Pre-allocated object pool (zero-alloc hot path)
├── tests/
│   └── test_order_book.cpp        # Unit tests for OrderBook (Google Test)
├── bench/
│   └── bench_order_book.cpp       # Latency benchmarks for OrderBook (Google Benchmark)
├── src/                           # C++ implementation files
└── docs/                          # Architecture & tuning docs (future)
```

---

## Key Design Decisions

### Fixed-Point Arithmetic
All monetary values use `int64_t` with a scale factor of `10^8`, not `double`. Floating-point arithmetic is non-deterministic (`0.1 + 0.2 ≠ 0.3`) — unacceptable when real money is at stake.

### Strong Types
`Price`, `Quantity`, `Timestamp`, `OrderId`, and `Duration` are all distinct types via a `StrongType<T, Tag>` wrapper. The compiler prevents accidentally passing a quantity where a price is expected.

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

---

## Build

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

**Requirements:** CMake 3.20+, C++20 compiler (MSVC 19.29+, GCC 11+, Clang 14+)

---

## Testing

Unit tests are written with [Google Test](https://github.com/google/googletest) and are fetched automatically by CMake.

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

Benchmarks use [Google Benchmark](https://github.com/google/benchmark) and are also fetched automatically by CMake.

```bash
# From the build directory
cmake --build . --target bench_order_book
./bench/bench_order_book
```

Current benchmarks (`bench/bench_order_book.cpp`):

| Benchmark | What it measures | stats on my end |
|---|---|---|
| `BM_OrderBookUpdate` | Throughput of `update()` with scattered prices (1M iterations) | 11-12 ns |
| `BM_OrderBookBestBidRead` | Raw read latency of `best_bid()` (10M iterations) | 0.114-0.156 ns |

---

## Roadmap

- [x] **Phase 1** — Core types, clock, ring buffer, memory pool
- [x] **Phase 2** — High-performance order book + unit tests + benchmarks
- [ ] **Phase 3** — Exchange connectivity & feed handlers (Binance WebSocket)
- [ ] **Phase 4** — Arbitrage detection strategy
- [ ] **Phase 5** — Order execution layer
- [ ] **Phase 6** — Risk management & kill switch
- [ ] **Phase 7** — Integration, optimization, kernel bypass (Linux)

---

## License

MIT
