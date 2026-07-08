#pragma once

#include "arb/core/order_book.hpp"
#include "arb/net/market_messages.hpp"
#include "arb/net/udp_receiver.hpp"
#include <iostream>

namespace arb::net {

class FeedHandler {
public:
  // We take a reference to the receiver and the book
  FeedHandler(UdpReceiver &receiver, OrderBook<32, 16> &book)
      : receiver_{receiver}, book_{book} {}

  void run_once() {
    // 1. Create a buffer on the stack
    alignas(64) uint8_t buffer[1024];

    // 2. Read bytes from the network
    std::size_t bytes_read = receiver_.poll(buffer, sizeof(buffer));

    if (bytes_read == 0) {
      return;
    }

    // 3. got data... now we will check header
    auto *header = reinterpret_cast<MessageHeader *>(buffer);

    // 4. Route based on message type
    if (header->msg_type == 'A') {
      // Add Order
      auto *msg = reinterpret_cast<AddOrderMessage *>(buffer);

      book_.update(msg->side, msg->price, msg->qty);

      std::cout << "--> RECEIVED ORDER: "
                << "Side=" << (int)msg->side << " Price=" << msg->price.get()
                << " Qty=" << msg->qty.get() << "\n";
    } else if (header->msg_type == 'M') {
      std::cout << "feature not available yet" << std::endl;
    } else if (header->msg_type == 'D') {
      std::cout << "feature not available yet" << std::endl;
    }
  }

private:
  UdpReceiver &receiver_;
  OrderBook<32, 16> &book_;
};

} // namespace arb::net