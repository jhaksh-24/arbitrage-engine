# CELAE — Cross-Exchange Latency Arbitrage Engine

A high-performance, low-latency cross-exchange arbitrage engine built in **C++20** with an **FPGA-accelerated order book** (Verilog). Designed to detect and exploit price discrepancies across cryptocurrency exchanges at nanosecond-level speeds.

> **Status:** Phase 2 — FPGA Order Book (in progress)

## Architecture

```
┌─────────────────────────────────────────────────────┐
│                  STRATEGY LAYER                      │
│  Arbitrage detection, signal generation, position    │
│  management, risk checks                            │
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

## Directory Structure

```
CELAE/
├── CMakeLists.txt                 # Build configuration (CMake 3.20+, C++20)
├── apps/
│   └── main.cpp                   # Entry point
├── include/arb/
│   ├── core/
│   │   └── types.hpp              # Fundamental types: Price, Quantity, Timestamp, OrderId
│   └── utils/
│       ├── clock.hpp              # High-resolution timing (steady_clock, ScopedTimer)
│       ├── ring_buffer.hpp        # Lock-free SPSC ring buffer for inter-thread comms
│       └── memory_pool.hpp        # Pre-allocated object pool (zero-alloc hot path)
├── rtl/                           # Verilog HDL source (FPGA order book)
│   ├── order_book.v               # Top-level order book module
│   ├── price_level.v              # Single price level register pair
│   └── comparator.v               # Parallel price comparator array
├── tb/                            # Verilog testbenches
│   └── tb_order_book.v            # Order book simulation testbench
├── src/                           # C++ implementation files (future phases)
├── tests/                         # C++ unit tests (future)
├── bench/                         # Latency benchmarks (future)
└── docs/                          # Architecture & tuning docs (future)
```

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

### FPGA-Accelerated Order Book
The order book is implemented in Verilog for hardware-level performance:
- Parallel price comparison across all levels in a single clock cycle
- Sorted insertion via shift registers — O(1) clock cycles
- Target latency: ~3 clock cycles (~15ns at 200MHz) vs ~200-500ns in C++

## Build

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

**Requirements:** CMake 3.20+, C++20 compiler (MSVC 19.29+, GCC 11+, Clang 14+)

## Roadmap

- [x] **Phase 1** — Core types, clock, ring buffer, memory pool
- [ ] **Phase 2** — FPGA order book (Verilog HDL, simulated via Icarus Verilog)
- [ ] **Phase 3** — Exchange connectivity & feed handlers (Binance WebSocket)
- [ ] **Phase 4** — Arbitrage detection strategy
- [ ] **Phase 5** — Order execution layer
- [ ] **Phase 6** — Risk management & kill switch
- [ ] **Phase 7** — Integration, optimization, kernel bypass (Linux)

## License

MIT
