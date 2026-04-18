#pragma once
#include "arb/core/types.hpp"

namespace arb {

struct Order {
  [[nodiscard]]
  constexpr bool is_terminal() const noexcept {
    return arb::is_terminal(status_);
  }

  [[nodiscard]]
  constexpr Quantity remaining_qty() const noexcept {
    return Quantity{quantity_.get() - filled_qty_.get()};
  }

  OrderId id_;
  Exchange exchange_;
  Side side_;
  Price price_;
  Quantity quantity_;
  Quantity filled_qty_{Quantity{0}};
  OrderType type_;
  OrderStatus status_{OrderStatus::NEW};
  Timestamp created_at_;
};

} // namespace arb