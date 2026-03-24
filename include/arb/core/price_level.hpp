#pragma once

#include "arb/core/types.hpp"

namespace arb {
struct PriceLevel {
public:
  constexpr PriceLevel() noexcept = default;
  explicit constexpr PriceLevel(Price price, Quantity quantity) noexcept
      : price_(price), quantity_(quantity) {}

  constexpr auto operator<=>(const PriceLevel &) const = default;

  [[nodiscard]]
  constexpr bool empty() const noexcept {
    return price_ == INVALID_PRICE;
  }

  [[nodiscard]]
  constexpr Price getPrice() const noexcept {
    return price_;
  }

  [[nodiscard]]
  constexpr Quantity getQty() const noexcept {
    return quantity_;
  }

  constexpr void setQty(Quantity qty) noexcept { quantity_ = qty; }

private:
  Price price_{INVALID_PRICE};
  Quantity quantity_{0};
};
} // namespace arb