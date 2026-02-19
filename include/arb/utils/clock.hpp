#pragma once

#include "arb/core/types.hpp"
#include <chrono>
#include <cstdint>
#include <format>
#include <string>

namespace arb {

[[nodiscard]]
inline arb::Timestamp now() noexcept {
  auto tp = std::chrono::steady_clock::now();
  auto duration = tp.time_since_epoch();

  auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
  auto count = ns.count();

  return arb::Timestamp{static_cast<std::uint64_t>(count)};
}

class ScopedTimer {
public:
  inline ScopedTimer(arb::Duration &out) noexcept : out_(out), start(now()) {}

  ~ScopedTimer() noexcept { out_ = now() - start; }

private:
  arb::Duration &out_;
  arb::Timestamp start;
};

[[nodiscard]]
inline std::string nanos_to_string(arb::Duration time) {
  return std::format("{} ns", time.get());
}

} // namespace arb