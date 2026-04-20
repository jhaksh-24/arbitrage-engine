#include "arb/core/order_book.hpp"
#include <benchmark/benchmark.h>

using namespace arb;

inline Price P(int64_t val) { return Price{val}; }
inline Quantity Q(int64_t val) { return Quantity{val}; }

static void BM_OrderBookUpdate(benchmark::State& state) {
  OrderBook<32, 16> book;
  int64_t i = 0;
  
  for (auto _ : state) {
    int64_t fake_price = (i * 7) % 500;
    book.update(Side::BUY, P(1000 + fake_price), Q(10));
    ++i;
  }
}
// Run the benchmark multiple times and average to get stable numbers
BENCHMARK(BM_OrderBookUpdate)->Iterations(1000000);

static void BM_OrderBookBestBidRead(benchmark::State& state) {
  OrderBook<32, 16> book;
  
  // Pre-fill the book so it has data to read
  book.update(Side::BUY, P(100), Q(50));
  
  for (auto _ : state) {
    // benchmark::DoNotOptimize forces the compiler to perform the read
    // and prevents dead code elimination
    benchmark::DoNotOptimize(book.best_bid().getPrice());
  }
}
BENCHMARK(BM_OrderBookBestBidRead)->Iterations(10000000);

// Automatically generate the main function for Google Benchmark
BENCHMARK_MAIN();