#pragma once
#include "arb/core/price_level.hpp"
#include "arb/core/types.hpp"
#include <algorithm>
#include <array>
#include <cstddef>

namespace arb {

// BookSide — one side (bids or asks) of a chunked-array order book.
//
// The price space is divided into fixed-size chunks. Each chunk holds
// a small sorted array of PriceLevels. This gives O(1) chunk lookup,
// bounded O(ChunkSize) shifts on insert/remove, and cache-friendly
// sequential access within each chunk.
//
// Template params:
//   ChunkSize — max price levels per chunk (default 32)
//   MaxChunks — max number of chunks (default 16)
//   Total capacity = ChunkSize * MaxChunks (default 512 levels per side)
template <std::size_t ChunkSize = 32, std::size_t MaxChunks = 16>
class BookSide {
  struct Chunk {
    std::array<PriceLevel, ChunkSize> levels{};
    std::size_t count{0}; // active levels in this chunk
  };

public:
  // Returns the best price level (index 0 of chunk 0).
  // Bids: highest price. Asks: lowest price.
  [[nodiscard]]
  constexpr const PriceLevel &best() const noexcept {
    return chunks_[0].levels[0];
  }

  [[nodiscard]]
  constexpr PriceLevel &best() noexcept {
    return chunks_[0].levels[0];
  }

  // Total active price levels across all chunks.
  [[nodiscard]]
  constexpr std::size_t size() const noexcept {
    std::size_t sum{};
    for (std::size_t i = 0; i < chunk_count_; ++i) {
      sum += chunks_[i].count;
    }
    return sum;
  }

  // Reset the book side — marks all chunks inactive.
  void clear() noexcept { chunk_count_ = 0; }

private:
  std::array<Chunk, MaxChunks> chunks_;
  std::size_t chunk_count_{0}; // number of active chunks
  bool is_bid_{false};         // true = bids (descending), false = asks (ascending)
};

} // namespace arb