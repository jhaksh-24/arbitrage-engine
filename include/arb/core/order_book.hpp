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
  explicit BookSide(bool is_bid) noexcept : is_bid_{is_bid} {}

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

  void set_level(Price price, Quantity qty) noexcept {
    for (std::size_t i = 0; i < chunk_count_; ++i) {
      auto &chunk = chunks_[i];
      bool found{false};

      if (chunk.levels[chunk.count - 1].getPrice() > price) {
        for (auto &level : chunk.levels) {
          if (level.getPrice() == price) {
            level.setQty(qty);
            found = true;
            return;
          }
        }
        if (!found) {
          insert_level(price, qty, chunk);
          break;
        }
      } else if (chunk.levels[chunk.count - 1].getPrice() == price) {
        chunk.levels[chunk.count - 1].setQty(qty);
        break;
      }
    }
  }

  Chunk &insert_level(Price price, Quantity qty, Chunk &chunk) noexcept {
    if (chunk.count == ChunkSize) {
      return chunk_split(price, qty, chunk);
    }

    std::size_t idx{};
    if (is_bid_) {
      for (; idx < chunk.count; idx++) {
        if (idx == 0 && chunk.levels[idx].getPrice() < price) {
          break;
        } else if (idx + 1 < chunk.count &&
                   chunk.levels[idx].getPrice() > price &&
                   chunk.levels[idx + 1].getPrice() < price) {
          idx++;
          break;
        }
      }
    }

    else {
      for (; idx < chunk.count; idx++) {
        if (idx == 0 && chunk.levels[idx].getPrice() > price) {
          break;
        } else if (idx + 1 < chunk.count &&
                   chunk.levels[idx].getPrice() < price &&
                   chunk.levels[idx + 1].getPrice() > price) {
          idx++;
          break;
        }
      }
    }

    for (std::size_t index{chunk.count}; index > idx; --index) {
      chunk.levels[index] = chunk.levels[index - 1];
    }

    chunk.levels[idx] = PriceLevel{price, qty};
    chunk.count++;

    return chunk;
  }

  Chunk &chunk_split(Price price, Quantity qty, Chunk &full_chunk) noexcept {
    if (chunk_count_ == MaxChunks) {
      return full_chunk;
    }

    std::size_t idx = &full_chunk - &chunks_[0];

    for (std::size_t index{chunk_count_}; index > idx; --index) {
      chunks_[index] = chunks_[index - 1];
    }
    std::size_t mid{ChunkSize / 2};
    std::array<PriceLevel, ChunkSize> levels;

    std::copy(full_chunk.levels.begin() + mid, full_chunk.levels.end(),
              levels.begin());

    Chunk newChunk{levels, ChunkSize - mid};
    full_chunk.count = mid;
    chunks_[idx + 1] = newChunk;
    chunk_count_++;

    if (is_bid_ ? (price > chunks_[idx + 1].levels[0].getPrice())
                : (price < chunks_[idx + 1].levels[0].getPrice())) {
      return insert_level(price, qty, chunks_[idx]);
    } else {
      return insert_level(price, qty, chunks_[idx + 1]);
    }
  }

private:
  std::array<Chunk, MaxChunks> chunks_;
  std::size_t chunk_count_{0}; // number of active chunks
  bool is_bid_{false}; // true = bids (descending), false = asks (ascending)
};

template <std::size_t ChunkSize = 32, std::size_t MaxChunks = 16>
class OrderBook {
public:
  explicit OrderBook() noexcept : bids_{true}, asks_{false} {}

  void update(Side side, Price price, Quantity qty) {
    if (side == Side::BUY) {
      bids_.set_level(price, qty);
    } else {
      asks_.set_level(price, qty);
    }
  }

  [[nodiscard]]
  constexpr const PriceLevel &best_bid() const noexcept {
    return bids_.best();
  }

  [[nodiscard]]
  constexpr const PriceLevel &best_ask() const noexcept {
    return asks_.best();
  }

  [[nodiscard]]
  constexpr Price spread() const noexcept {
    return Price{best_ask().getPrice().get() - best_bid().getPrice().get()};
  }

  void clear() noexcept {
    bids_.clear();
    asks_.clear();
  }

  void clear_bids() noexcept { bids_.clear(); }

  void clear_asks() noexcept { asks_.clear(); }

private:
  BookSide<ChunkSize, MaxChunks> bids_;
  BookSide<ChunkSize, MaxChunks> asks_;
};

} // namespace arb
