#pragma once

#include <cstdint>
#include <string_view>

namespace arb {

inline constexpr std::int64_t PRICE_SCALE = 100'000'000LL;

template <typename T, typename Tag> struct StrongType {
public:
  explicit constexpr StrongType(T v) noexcept : value_(v) {}

  [[nodiscard]]
  constexpr T get() const noexcept {
    return value_;
  }

  constexpr auto operator<=>(const StrongType &) const = default;

private:
  T value_;
};

struct PriceTag {};
struct QtyTag {};
struct TimestampTag {};
struct OrderIdTag {};
struct DurationTag {};

using Price = StrongType<std::int64_t, PriceTag>;
using Quantity = StrongType<std::int64_t, QtyTag>;
using Timestamp = StrongType<std::uint64_t, TimestampTag>;
using OrderId = StrongType<std::uint64_t, OrderIdTag>;
using Duration = StrongType<std::uint64_t, DurationTag>;

inline constexpr Duration operator-(Timestamp a, Timestamp b) noexcept {
  return Duration{a.get() - b.get()};
}

inline constexpr Timestamp operator+(Timestamp t, Duration d) noexcept {
  return Timestamp{t.get() + d.get()};
}

enum class Side : std::uint8_t {
  BUY = 0,
  SELL = 1,
};

[[nodiscard]]
inline constexpr Side opposite(Side s) noexcept {
  return s == Side::BUY ? Side::SELL : Side::BUY;
}

[[nodiscard]]
inline constexpr std::string_view to_string(Side s) noexcept {
  return s == Side::BUY ? "BUY" : "SELL";
}

enum class Exchange : std::uint8_t {
  BINANCE = 0,
  BYBIT = 1,
  OKX = 2,
  COUNT = 3,
};

enum class OrderStatus : std::uint8_t {
  NEW = 0,              // Created locally, not yet sent
  SENT = 1,             // Sent to exchange, awaiting acknowledgment
  ACKED = 2,            // Exchange acknowledged, order is live
  PARTIALLY_FILLED = 3, // Some quantity filled, rest still live
  FILLED = 4,           // Fully filled — terminal state
  CANCELLED = 5,        // We cancelled or exchange cancelled — terminal state
  REJECTED = 6,         // Exchange rejected the order — terminal state
  ERROR = 7,            // Something went wrong — terminal state
};

[[nodiscard]]
constexpr bool is_terminal(OrderStatus s) noexcept {
  return s == OrderStatus::FILLED || s == OrderStatus::CANCELLED ||
         s == OrderStatus::REJECTED || s == OrderStatus::ERROR;
}

enum class OrderType : std::uint8_t {
  LIMIT = 0,
  MARKET = 1,
};

[[nodiscard]]
constexpr Price to_fixed_price(double price) noexcept {
  return Price{static_cast<std::int64_t>(price * PRICE_SCALE)};
}

[[nodiscard]]
constexpr double to_double_price(Price price) noexcept {
  return static_cast<double>(price.get()) / static_cast<double>(PRICE_SCALE);
}

inline constexpr Price INVALID_PRICE{0};
inline constexpr Price INVALID_QUANTITY{0};
inline constexpr OrderId INVALID_ORDER_ID{0};
} // namespace arb