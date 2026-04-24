#pragma once
#include "arb/core/price_level.hpp"
#include "arb/core/types.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>

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
  void clear() noexcept {
    chunk_count_ = 0;
    chunks_[0].levels[0] =
        PriceLevel{}; // Clear the best level so best() returns empty
  }

  void set_level(Price price, Quantity qty) noexcept {
    if (qty.get() == 0) {
      remove_level(price);
      return;
    }

    if (chunk_count_ == 0) {
      chunk_count_ = 1;
      insert_level(price, qty, chunks_[0]);
      return;
    }

    for (std::size_t i = 0; i < chunk_count_; ++i) {
      auto &chunk = chunks_[i];

      // Determine if price belongs in or before this chunk
      bool fits_in_chunk =
          is_bid_ ? (chunk.levels[chunk.count - 1].getPrice() <= price)
                  : (chunk.levels[chunk.count - 1].getPrice() >= price);

      if (fits_in_chunk) {
        for (std::size_t j = 0; j < chunk.count; ++j) {
          if (chunk.levels[j].getPrice() == price) {
            chunk.levels[j].setQty(qty);
            return;
          }
        }
        // Not found, belongs here -> insert
        insert_level(price, qty, chunk);
        return;
      }
    }

    // Price is worse than everything in the book, append to the last chunk
    insert_level(price, qty, chunks_[chunk_count_ - 1]);
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

    std::size_t idx = static_cast<std::size_t>(&full_chunk - &chunks_[0]);

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
  void remove_level(Price price) noexcept {
    for (std::size_t i = 0; i < chunk_count_; ++i) {
      auto &chunk = chunks_[i];

      // Determine if price might be in this chunk
      bool might_be_in_chunk =
          is_bid_ ? (chunk.levels[chunk.count - 1].getPrice() <= price)
                  : (chunk.levels[chunk.count - 1].getPrice() >= price);

      if (might_be_in_chunk) {
        for (std::size_t j = 0; j < chunk.count; ++j) {
          if (chunk.levels[j].getPrice() == price) {

            // 1. Shift elements left to overwrite the deleted level
            if (j < chunk.count - 1) {
              std::memmove(&chunk.levels[j], &chunk.levels[j + 1],
                           (chunk.count - j - 1) * sizeof(PriceLevel));
            }
            chunk.count--;

            // 2. If the chunk is now empty, shift the chunks array left to
            // avoid gaps
            if (chunk.count == 0) {
              if (i < chunk_count_ - 1) {
                std::memmove(&chunks_[i], &chunks_[i + 1],
                             (chunk_count_ - i - 1) * sizeof(Chunk));
              }
              chunk_count_--;
            }

            return;
          }
        }
      }
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
