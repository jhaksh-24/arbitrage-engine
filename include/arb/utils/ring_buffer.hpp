#pragma once

#include <array>
#include <atomic>
#include <cstddef>

namespace arb {

template <typename T, std::size_t Capacity> class RingBuffer {
  static_assert(Capacity > 0, "Capacity should be greater than zero");
  static_assert((Capacity & (Capacity - 1)) == 0,
                "Capacity should be in power of 2");

public:
  [[nodiscard]]
  constexpr std::size_t getCapacity() const noexcept {
    return Capacity;
  }

private:
  std::array<T, Capacity> buffer_;
  alignas(64) std::atomic<std::size_t> head_{0};
  alignas(64) std::atomic<std::size_t> tail_{0};
  static constexpr std::size_t mask = Capacity - 1;

public:
  [[nodiscard]]
  inline bool try_push(const T &item) noexcept {
    auto tail = tail_.load(std::memory_order_relaxed);
    auto head = head_.load(std::memory_order_acquire);

    auto next = (tail + 1) & mask;

    if (next == head) {
      return false;
    }

    buffer_[tail] = item;
    tail_.store(next, std::memory_order_release);
    return true;
  }

  [[nodiscard]]
  inline bool try_pop(T &item) noexcept {
    auto tail = tail_.load(std::memory_order_acquire);
    auto head = head_.load(std::memory_order_relaxed);

    auto next = (head + 1) & mask;

    if (tail == head) {
      return false;
    }

    item = buffer_[head];
    head_.store(next, std::memory_order_release);
    return true;
  }
};

} // namespace arb