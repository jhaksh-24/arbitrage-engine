#include "arb/core/order_book.hpp"
#include <arb/net/binance_feed.hpp>
#include <iostream>
#include <thread>

int main(void) {
  ix::initNetSystem();
  arb::OrderBook<32, 16> book;
  arb::BinanceFeed feed(book);
  feed.start("btcusdt");

  for (std::size_t idx{0}; idx < 30; ++idx) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    auto bid = book.best_bid();
    auto ask = book.best_ask();
    std::cout << "BID: " << bid.getPrice().get()
              << " | ASK: " << ask.getPrice().get() << "\n";
  }
  feed.stop();
  ix::uninitNetSystem();
}