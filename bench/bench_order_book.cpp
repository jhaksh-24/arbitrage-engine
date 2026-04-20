#include "arb/core/order_book.hpp"
#include "arb/utils/clock.hpp"
#include <cstdint>
#include <iostream>
#include <ostream>

using namespace arb;

inline Price P(int64_t val) { return Price{val}; }
inline Quantity Q(int64_t val) { return Quantity{val}; }

int main(void) {
  std::cout << "= = = [ Chunked array Order book Benchmark ] = = =" << "\n\n";

  OrderBook<32, 16> book;

  // ===================================
  // 1. Benchmark : insertion/update (going for 100k updates)
  // ===================================

  Duration update_duration{0};
  const int num_updates = 100'000;
  {
    ScopedTimer timer(update_duration);
    for (int i = 0; i < num_updates; ++i) {
      int64_t fake_price = (i * 7) % 500;
      book.update(Side::BUY, P(1000 + fake_price), Q(10));
    }
  }

  std::cout << "[update] 100k operations took "
            << nanos_to_string(update_duration) << "\n";
  std::cout << "[update] Average latency: "
            << update_duration.get() / num_updates << " ns/op\n\n";

  // ===================================
  // 2. Benchmark : Best Bid/Ask Read
  // ===================================

  Duration read_duration{0};
  const int num_reads = 1'000'000;

  {
    ScopedTimer timer(read_duration);
    for (int i = 0; i < num_reads; ++i) {
      auto volatile _ = book.best_bid().getPrice().get();
    }
  }

  std::cout << "[best_bid] 1M reads took " << nanos_to_string(read_duration)
            << "\n";
  std::cout << "[best_bid] Average latency: " << read_duration.get() / num_reads
            << " ns/op\n\n";
  return 0;
}