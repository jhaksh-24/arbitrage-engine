#pragma once

#include "arb/core/order_book.hpp"
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <string>

namespace arb {

class BinanceFeed {
public:
  explicit BinanceFeed(arb::OrderBook<32, 16> &order_book)
      : order_book_{order_book} {}

  void start(const std::string &symbol) noexcept;

  void stop(void) noexcept;

private:
  void on_message(const ix::WebSocketMessagePtr &msg) noexcept;

  void parse_depth_update(const std::string &payload) noexcept;

private:
  arb::OrderBook<32, 16> &order_book_;
  ix::WebSocket web_sock;
};

} // namespace arb