#include "arb/net/binance_feed.hpp"
#include "arb/core/order_book.hpp"
#include "arb/core/types.hpp"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <simdjson.h>
#include <string>
#include <string_view>

namespace arb {

void BinanceFeed::start(const std::string &symbol) noexcept {
  std::string base = "wss://stream.binance.com:9443/ws/";
  std::string event = "@depth@100ms";

  std::string url = base + symbol + event;

  web_sock.setUrl(url);
  web_sock.setOnMessageCallback(
      [this](const ix::WebSocketMessagePtr &msg) { this->on_message(msg); });
  web_sock.start();
}

void BinanceFeed::stop(void) noexcept { web_sock.stop(); }

void BinanceFeed::on_message(const ix::WebSocketMessagePtr &msg) noexcept {
  if (msg->type == ix::WebSocketMessageType::Message) {
    std::cout << "[RECIEVING MESSAGE] ";
    parse_depth_update(msg->str);
  } else if (msg->type == ix::WebSocketMessageType::Open) {
    std::cout << "[CONNECTION OPEN] " << "Connected to Binance" << "\n\n";
  } else if (msg->type == ix::WebSocketMessageType::Error) {
    std::cout << "[ERROR] " << msg->errorInfo.reason << "\n\n";
  }
}

void BinanceFeed::parse_depth_update(const std::string &payload) noexcept {
  static simdjson::ondemand::parser parser;
  auto doc_result = parser.iterate(payload);

  if (doc_result.error()) {
    std::cerr << "[ERROR] " << "Error parsing JSON: " << doc_result.error()
              << "\n\n";
    return;
  }

  simdjson::ondemand::document doc = doc_result.value();

  auto [bids, err_b] = doc["b"].get_array();
  if (err_b)
    return;

  for (auto bid : bids) {
    auto inner = bid.get_array().value();
    auto it = inner.begin();
    std::string_view price_sv = (*it).get_string().value();
    ++it;
    std::string_view qty_sv = (*it).get_string().value();

    double price_d = std::stod(std::string(price_sv));
    double qty_d = std::stod(std::string(qty_sv));

    Price price = Price{static_cast<int64_t>(price_d * 100)};
    Quantity qty = Quantity{static_cast<int64_t>(qty_d * 100000000)};

    order_book_.update(Side::BUY, price, qty);
  }

  auto [asks, err_a] = doc["a"].get_array();
  if (err_a)
    return;

  for (auto ask : asks) {
    auto inner = ask.get_array().value();
    auto it = inner.begin();
    std::string_view price_sv = (*it).get_string().value();
    ++it;
    std::string_view qty_sv = (*it).get_string().value();

    double price_d = std::stod(std::string(price_sv));
    double qty_d = std::stod(std::string(qty_sv));

    Price price = Price{static_cast<int64_t>(price_d * 100)};
    Quantity qty = Quantity{static_cast<int64_t>(qty_d * 100000000)};

    order_book_.update(Side::SELL, price, qty);
  }
}

} // namespace arb