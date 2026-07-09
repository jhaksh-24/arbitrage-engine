#include "arb/net/bybit_feed.hpp"
#include "arb/core/order_book.hpp"
#include "arb/core/types.hpp"
#include "arb/utils/logger.hpp"
#include <cstddef>
#include <cstdint>
#include <simdjson.h>
#include <string>
#include <string_view>

namespace arb {

void BybitFeed::start(const std::string &symbol) noexcept {
  std::string url = "wss://stream.bybit.com/v5/public/linear";

  web_sock.setUrl(url);

  std::string sub_msg =
      "{\"op\": \"subscribe\", \"args\": [\"orderbook.50." + symbol + "\"]}";

  web_sock.setOnMessageCallback(
      [this, sub_msg](const ix::WebSocketMessagePtr &msg) {
        if (msg->type == ix::WebSocketMessageType::Open) {
          arb::log::info("Connected to Bybit");
          this->web_sock.send(sub_msg);
          arb::log::info("Sent Bybit subscription: %s", sub_msg.c_str());
        } else if (msg->type == ix::WebSocketMessageType::Message) {
          this->parse_depth_update(msg->str);
        } else if (msg->type == ix::WebSocketMessageType::Error) {
          arb::log::error("Bybit Error: %s", msg->errorInfo.reason.c_str());
        }
      });

  web_sock.start();
}

void BybitFeed::stop(void) noexcept { web_sock.stop(); }

void BybitFeed::on_message(const ix::WebSocketMessagePtr &msg) noexcept {
  if (msg->type == ix::WebSocketMessageType::Message) {
    parse_depth_update(msg->str);
  } else if (msg->type == ix::WebSocketMessageType::Open) {
    arb::log::info("Connected to Bybit");
  } else if (msg->type == ix::WebSocketMessageType::Error) {
    arb::log::error("Bybit Error: %s", msg->errorInfo.reason.c_str());
  }
}

void BybitFeed::parse_depth_update(const std::string &payload) noexcept {
  static simdjson::ondemand::parser parser;
  simdjson::padded_string padded(payload);
  auto doc_result = parser.iterate(padded);

  if (doc_result.error()) {
    arb::log::error("Error parsing Bybit JSON: %s",
                    simdjson::error_message(doc_result.error()));
    return;
  }

  simdjson::ondemand::document doc = std::move(doc_result).value();

  simdjson::ondemand::object data;
  if (doc["data"].get_object().get(data))
    return;

  for (auto field : data) {
    if (field.key() == "b") {
      simdjson::ondemand::array bids;
      if (!field.value().get_array().get(bids)) {
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
      }
    } else if (field.key() == "a") {
      simdjson::ondemand::array asks;
      if (!field.value().get_array().get(asks)) {
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
    }
  }
}

} // namespace arb
