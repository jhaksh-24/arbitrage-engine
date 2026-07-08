# CELAE — Cross-Exchange Latency Arbitrage Engine

A high-performance, low-latency cross-exchange arbitrage engine built in **C++20**. Designed to detect and exploit price discrepancies across exchanges at microsecond-level speeds using raw UDP multicast feeds and zero-allocation binary parsing.

> **Status:** Phase 3 Complete — Low-Latency UDP Feed Handlers

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
│  UDP multicast, raw sockets, kernel bypass           │
└─────────────────────────────────────────────────────┘
```

### Data Flow (Phase 3)

```
UDP Multicast Packet
        │
        ▼
┌──────────────────┐
│   UdpReceiver    │  Non-blocking recv() into stack-allocated buffer
│   (poll loop)    │  Zero heap allocations on the hot path
└────────┬─────────┘
         │ raw bytes
         ▼
┌──────────────────┐
│   FeedHandler    │  reinterpret_cast<MessageHeader*>(buffer)
│   (run_once)     │  Routes by msg_type: 'A', 'M', 'D'
└────────┬─────────┘
         │ typed struct
         ▼
┌──────────────────┐
│    OrderBook     │  L2 chunked-array book
│   (update)       │  O(1) best bid/ask access
└──────────────────┘
```

---

## Directory Structure

```
CELAE/
├── CMakeLists.txt                 # Build configuration (CMake 3.20+, C++20)
├── mock_exchange.py               # Python script to blast binary UDP packets for testing
├── apps/
│   ├── main.cpp                   # Entry point
│   └── udp_feed_test.cpp          # Live test for UDP multicast feed handler
├── include/arb/
│   ├── core/
│   │   ├── types.hpp              # Fundamental types: Price, Quantity, Timestamp, OrderId
│   │   ├── price_level.hpp        # L2 price level (price + quantity aggregate)
│   │   ├── order.hpp              # Order struct (aggregate for execution layer)
│   │   └── order_book.hpp         # Chunked-array order book (BookSide, OrderBook)
│   ├── net/
│   │   ├── market_messages.hpp    # Packed binary protocol structs (ITCH/SBE style)
│   │   ├── udp_receiver.hpp       # Zero-allocation multicast UDP socket wrapper
│   │   └── feed_handler.hpp       # L2 Book updater via reinterpret_cast
│   └── utils/
│       ├── clock.hpp              # High-resolution timing (steady_clock, ScopedTimer)
│       ├── ring_buffer.hpp        # Lock-free SPSC ring buffer for inter-thread comms
│       └── memory_pool.hpp        # Pre-allocated object pool (zero-alloc hot path)
├── tests/
│   └── test_order_book.cpp        # Unit tests for OrderBook (Google Test)
├── bench/
│   └── bench_order_book.cpp       # Latency benchmarks for OrderBook (Google Benchmark)
├── src/                           # C++ implementation files
│   └── net/
│       └── udp_receiver.cpp       # Non-blocking UDP multicast implementation
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

### Binary Protocol & Zero-Copy Parsing (Phase 3)
The feed layer replaces the earlier JSON/WebSocket approach with a custom packed binary protocol inspired by real exchange formats like NASDAQ ITCH 5.0 and CME SBE:
- All message structs use `#pragma pack(push, 1)` to eliminate compiler padding, ensuring the in-memory layout matches the wire format byte-for-byte
- Incoming UDP payloads are parsed via `reinterpret_cast` — zero string parsing, zero intermediate copies, zero heap allocations
- Each message starts with a `MessageHeader` (type + length + nanosecond timestamp), followed by type-specific fields (`AddOrderMessage`, `ModifyOrderMessage`, `DeleteOrderMessage`)

### UDP Multicast Receiver (Phase 3)
The network layer uses raw POSIX/Winsock UDP sockets rather than a high-level library:
- Non-blocking `recv()` via `MSG_DONTWAIT` (Linux) or `ioctlsocket` with `FIONBIO` (Windows)
- Multicast group join via `IP_ADD_MEMBERSHIP` — the OS-level mechanism for subscribing to exchange feed groups
- `SO_REUSEADDR` enabled for multi-process debugging
- Stack-allocated `alignas(64)` receive buffer — aligned to CPU cache line boundaries for optimal memory access
- Cross-platform: compiles on both Windows (Winsock2 / `ws2_32`) and Linux (POSIX sockets)

---

## Build

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

**Requirements:** CMake 3.20+, C++20 compiler (MSVC 19.29+, GCC 11+, Clang 14+)

---

## Running the Feed Engine

Start the UDP feed listener in one terminal:

```bash
# From the build directory
./Debug/udp_feed_test    # Windows
./udp_feed_test          # Linux
```

In a second terminal, fire test packets using the mock exchange:

```bash
# From the project root
python mock_exchange.py
```

The engine should print received orders:

```
Starting HFT UDP Feed Engine...
Listening for packets on 239.255.0.1:12345...
--> RECEIVED ORDER: Side=0 Price=5000000 Qty=100000000
```

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
| `BM_OrderBookUpdate` | Throughput of `update()` with scattered prices (100k iterations) | 11-12 ns |
| `BM_OrderBookBestBidRead` | Raw read latency of `best_bid()` (1M iterations) | 0.114-0.156 ns |

---

## Known Limitations

These are deliberate trade-offs in the current implementation, not bugs:

| Limitation | Why | Path Forward |
|---|---|---|
| **L2 book only** — Modify (`'M'`) and Delete (`'D'`) messages are received but not processed | The order book tracks aggregate quantity per price level, not individual order IDs. Processing Modify/Delete requires an L3 Order Tracker (`unordered_map<OrderId, Order>`) to remember the side and price of every order | Build an `OrderTracker` in Phase 4 that maps `OrderId → {Side, Price, Qty}` and bridges L3 events to L2 book updates |
| **Custom binary protocol** — Not wired to a real exchange yet | We use a proprietary message format for development and benchmarking. Real exchanges use ITCH, PITCH, or SBE | Write protocol-specific struct definitions (e.g., `nasdaq_itch_messages.hpp`) and a translator layer. Historical PCAP replay can be used for testing against real market data |
| **No sequence number / gap detection** — Lost UDP packets are silently missed | UDP is unreliable by design; the current receiver does not track sequence numbers | Add a `seq_num` field to `MessageHeader` and implement gap detection with snapshot recovery |
| **Single-threaded hot loop** — `FeedHandler::run_once()` runs on the caller's thread | Sufficient for current throughput; not yet pinned to an isolated CPU core | Pin the feed thread to a dedicated core via `pthread_setaffinity_np` (Linux) or `SetThreadAffinityMask` (Windows), and use the existing `RingBuffer` to decouple the feed thread from the strategy thread |

---

## Roadmap

- [x] **Phase 1** — Core types, clock, ring buffer, memory pool
- [x] **Phase 2** — High-performance order book + unit tests + benchmarks
- [x] **Phase 3** — Low-latency UDP Multicast binary feed handlers
- [ ] **Phase 4** — Arbitrage detection strategy
- [ ] **Phase 5** — Order execution layer
- [ ] **Phase 6** — Risk management & kill switch
- [ ] **Phase 7** — Integration, optimization, kernel bypass (Linux)

---

## License

MIT
