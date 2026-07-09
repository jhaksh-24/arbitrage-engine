#include "arb/core/order_book.hpp"
#include "arb/net/binance_feed.hpp"
#include "arb/net/bybit_feed.hpp"
#include "arb/strategy/arbitrage_strategy.hpp"
#include "arb/utils/logger.hpp"
#include <thread>

int main(void) {
  ix::initNetSystem();

  arb::OrderBook<32, 16> binance_book;
  arb::OrderBook<32, 16> bybit_book;

  arb::BinanceFeed binance(binance_book);
  arb::BybitFeed bybit(bybit_book);
  
  // Initialize strategy with 0.0 fees so we can see the raw arbitrage simulation trigger!
  arb::ArbitrageStrategy strategy(binance_book, bybit_book, 0.0, 0.0);

  arb::log::info("Starting Phase 3.5: Dual Exchange Live Feed");

  // Start both feeds (BTCUSDT)
  binance.start("btcusdt"); // Binance requires lowercase
  bybit.start("BTCUSDT");   // Bybit requires uppercase

  for (std::size_t idx{0}; idx < 30; ++idx) {
    std::this_thread::sleep_for(std::chrono::seconds(1));

    auto binance_bid = binance_book.best_bid().getPrice().get();
    auto binance_ask = binance_book.best_ask().getPrice().get();
    
    auto bybit_bid = bybit_book.best_bid().getPrice().get();
    auto bybit_ask = bybit_book.best_ask().getPrice().get();

    arb::log::info("BINANCE  | BID: %10lld | ASK: %10lld", binance_bid, binance_ask);
    arb::log::info("BYBIT    | BID: %10lld | ASK: %10lld", bybit_bid, bybit_ask);
    
    // Evaluate cross-exchange arbitrage strategy
    strategy.evaluate();
    
    arb::log::info("-------------------------------------------------");
  }

  binance.stop();
  bybit.stop();
  
  ix::uninitNetSystem();
  return 0;
}