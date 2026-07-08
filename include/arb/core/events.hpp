#pragma once
#include "arb/core/types.hpp"

namespace arb {

struct DepthUpdateEvent {
  Side side;
  Price price;
  Quantity qty;

  constexpr DepthUpdateEvent() noexcept : side{Side::BUY}, price{0}, qty{0} {}

  constexpr DepthUpdateEvent(Side s, Price p, Quantity q) noexcept
      : side{s}, price{p}, qty{q} {}
};

} // namespace arb