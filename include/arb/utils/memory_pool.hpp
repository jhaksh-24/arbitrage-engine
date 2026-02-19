#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <numeric>

namespace arb {

template <typename T, std::size_t PoolSize> class MemoryPool {
  static_assert(
      PoolSize > 0,
      "Pool size should be greater than zero and should not be negative");

public:
  MemoryPool() : top_(PoolSize) {
    std::iota(free_list_.begin(), free_list_.end(), std::size_t{0});
  }

  [[nodiscard]]
  T *allocate() noexcept {
    if (top_ == 0)
      return nullptr;

    std::size_t index = free_list_[--top_];
    return &pool_[index];
  }

  void deallocate(T *ptr) noexcept {
    if (!ptr)
      return;

    std::size_t index = static_cast<std::size_t>(ptr - &pool_[0]);
    assert(index < PoolSize);
    assert(top_ < PoolSize);
    free_list_[top_++] = index;
  }

private:
  std::array<T, PoolSize> pool_;
  std::array<std::size_t, PoolSize> free_list_;
  std::size_t top_;
};

} // namespace arb