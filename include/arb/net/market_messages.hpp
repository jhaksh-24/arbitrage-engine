#pragma once

#include "arb/core/types.hpp"
#include <cstdint>

namespace arb::net {

// Enforce 1 byte alignment (no padding)
#pragma pack(push, 1)

// Every message starts with a header so we know what type it is
struct MessageHeader {
  uint8_t msg_type;   // A-add, M-Modify, D-Delete
  uint16_t length;    // Total length of message in bytes
  uint64_t timestamp; // Exchange timestamp in nanoseconds
};

// A-add order
struct AddOrderMessage {
  MessageHeader header;
  OrderId order_id;
  Side side;
  Price price;
  Quantity qty;
};

// M-modify order
struct ModifyOrderMessage {
  MessageHeader header;
  OrderId order_id;
  Price price;
  Quantity qty;
};

// D-Delete order
struct DeleteOrderMessage {
  MessageHeader header;
  OrderId order_id;
};

// Restore default compiler alignment
#pragma pack(pop)

} // namespace arb::net