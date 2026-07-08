#include "arb/core/order_book.hpp"
#include "arb/net/feed_handler.hpp"
#include "arb/net/udp_receiver.hpp"
#include <iostream>

int main() {
  try {
    std::cout << "Starting HFT UDP Feed Engine..." << std::endl;

    // 1. Initialize the core Order Book
    arb::OrderBook<32, 16> book;

    // 2. Initialize the UDP Receiver (Listening on Multicast IP 239.255.0.1,
    // Port 12345)
    arb::net::UdpReceiver receiver("239.255.0.1", 12345);

    // 3. Initialize the Feed Handler
    arb::net::FeedHandler feed(receiver, book);

    std::cout << "Listening for packets on 239.255.0.1:12345..." << std::endl;

    // 4. The HFT Hot Loop!
    // In a real system, this loop runs on an isolated CPU core
    // that is 100% dedicated to spinning as fast as possible.
    while (true) {
      feed.run_once();
    }

  } catch (const std::exception &e) {
    std::cerr << "Fatal Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
