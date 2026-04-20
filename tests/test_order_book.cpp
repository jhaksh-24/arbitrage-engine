#include "arb/core/order_book.hpp"
#include "arb/core/types.hpp"
#include <gtest/gtest.h>

inline arb::Price P(int64_t val) { return arb::Price{val}; }
inline arb::Quantity Q(int64_t val) { return arb::Quantity{val}; }

using namespace arb;

// =======================================
// Test 1: A fresh order book should have empty price levels
// =======================================

TEST(OrderBookTest, EmptyBook) {
  OrderBook<32, 16> book;

  EXPECT_EQ(book.best_bid().getPrice(), P(0));
  EXPECT_EQ(book.best_ask().getPrice(), P(0));
}

// =======================================
// Test 2: Inserting a single Bid
// =======================================

TEST(OrderBookTest, SingleBidInsert) {
  OrderBook<32, 16> book;

  book.update(Side::BUY, P(100), Q(10));
  EXPECT_EQ(book.best_bid().getPrice(), P(100));
  EXPECT_EQ(book.best_bid().getQty(), Q(10));
  EXPECT_EQ(book.best_ask().getPrice(), P(0));
}

// =======================================
// Test 3: Inserting a single Ask
// =======================================

TEST(OrderBookTest, SingleAskInsert) {
  OrderBook<32, 16> book;

  book.update(Side::SELL, P(105), Q(5));
  EXPECT_EQ(book.best_ask().getPrice(), P(105));
  EXPECT_EQ(book.best_ask().getQty(), Q(5));
  EXPECT_EQ(book.best_bid().getPrice(), P(0));
}

// =======================================
// Test 4: Multiple Bids Sorting
// =======================================

TEST(OrderBookTest, MultipleBidsSorting) {
  OrderBook<32, 16> book;

  book.update(Side::BUY, P(90), Q(5));
  book.update(Side::BUY, P(100), Q(10)); // Should be best bid
  book.update(Side::BUY, P(95), Q(7));

  EXPECT_EQ(book.best_bid().getPrice(), P(100));
  EXPECT_EQ(book.best_bid().getQty(), Q(10));
}

// =======================================
// Test 5: Multiple Asks Sorting
// =======================================

TEST(OrderBookTest, MultipleAsksSorting) {
  OrderBook<32, 16> book;

  book.update(Side::SELL, P(95), Q(5));
  book.update(Side::SELL, P(84), Q(10)); // Should be the best ask
  book.update(Side::SELL, P(90), Q(7));

  EXPECT_EQ(book.best_ask().getPrice(), P(84));
  EXPECT_EQ(book.best_ask().getQty(), Q(10));
}

// =======================================
// Test 6: Quantity updation check on same prices
// =======================================

TEST(OrderBookTest, UpdateLevel) {
  OrderBook<32, 16> book;

  book.update(Side::BUY, P(76), Q(8));
  book.update(Side::SELL, P(80), Q(4));
  book.update(Side::BUY, P(76), Q(10));
  book.update(Side::SELL, P(80), Q(7));

  EXPECT_EQ(book.best_bid().getPrice(), P(76));
  EXPECT_EQ(book.best_bid().getQty(), Q(10)); // Replacing qty, not adding!
  EXPECT_EQ(book.best_ask().getPrice(), P(80));
  EXPECT_EQ(book.best_ask().getQty(), Q(7));
}

// =======================================
// Test 7: Spread Calculation
// =======================================

TEST(OrderBookTest, SpreadCalc) {
  OrderBook<32, 16> book;

  book.update(Side::BUY, P(100), Q(5));
  book.update(Side::SELL, P(105), Q(7));

  EXPECT_EQ(book.spread(), P(5));
}

// =======================================
// Test 8: Clear Book
// =======================================

TEST(OrderBookTest, ClearBook) {
  OrderBook<32, 16> book;

  book.update(Side::BUY, P(76), Q(8));
  book.update(Side::SELL, P(80), Q(4));

  book.clear();

  EXPECT_EQ(book.best_bid().getPrice(), P(0));
  EXPECT_EQ(book.best_ask().getPrice(), P(0));
}

// =======================================
// Test 9: Cross Chunk Insert (Forcing Chunk Split)
// =======================================

TEST(OrderBookTest, CrossChunkInsert) {
  OrderBook<32, 16> book;

  // Insert 40 bids (prices 101 to 140)
  for (int i = 1; i <= 40; ++i) {
    book.update(Side::BUY, P(100 + i), Q(5));
  }

  // Insert 40 asks (prices 151 to 190)
  for (int i = 1; i <= 40; ++i) {
    book.update(Side::SELL, P(150 + i), Q(5));
  }

  // Best Bid should be the highest (140)
  // Best Ask should be the lowest (151)
  EXPECT_EQ(book.best_bid().getPrice(), P(140));
  EXPECT_EQ(book.best_ask().getPrice(), P(151));
}