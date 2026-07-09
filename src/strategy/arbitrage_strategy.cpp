#include "arb/strategy/arbitrage_strategy.hpp"
#include "arb/core/types.hpp"
#include "arb/utils/logger.hpp"
#include "arb/utils/clock.hpp"
#include <algorithm>

namespace arb {

ArbitrageStrategy::ArbitrageStrategy(OrderBook<32, 16> &binance,
                                     OrderBook<32, 16> &bybit,
                                     double binance_fee, double bybit_fee)
    : binance_(binance), bybit_(bybit), 
      binance_fee_(binance_fee), bybit_fee_(bybit_fee) {}

void ArbitrageStrategy::evaluate() noexcept {
  arb::Duration eval_latency{0};
  
  {
    arb::ScopedTimer timer(eval_latency);
    
    auto bin_bid = binance_.best_bid();
    auto bin_ask = binance_.best_ask();
    auto byb_bid = bybit_.best_bid();
    auto byb_ask = bybit_.best_ask();

    // Ensure we have real prices on both books before doing math
    if (bin_bid.getPrice().get() == 0 || bin_ask.getPrice().get() == 0 ||
        byb_bid.getPrice().get() == 0 || byb_ask.getPrice().get() == 0) {
      return;
    }

    // SCENARIO 1: Buy Bybit, Sell Binance
    // (Bybit Ask is cheaper than Binance Bid)
    int64_t buy_price_1 = byb_ask.getPrice().get();
    int64_t sell_price_1 = bin_bid.getPrice().get();

    double fee_1 = (buy_price_1 * bybit_fee_) + (sell_price_1 * binance_fee_);
    double net_profit_1 = (sell_price_1 - buy_price_1) - fee_1;

    if (net_profit_1 > 0) {
      int64_t trade_qty = std::min(byb_ask.getQty().get(), bin_bid.getQty().get());
      arb::log::info("[ARB DETECTED] Buy Bybit / Sell Binance | Profit Ticks: %.2f | Qty: %lld", net_profit_1, trade_qty);
      
      // Simulate Trade: Subtract volume from local books so we don't spam the same signal
      bybit_.update(Side::SELL, byb_ask.getPrice(), Quantity{byb_ask.getQty().get() - trade_qty});
      binance_.update(Side::BUY, bin_bid.getPrice(), Quantity{bin_bid.getQty().get() - trade_qty});
    }

    // SCENARIO 2: Buy Binance, Sell Bybit
    // (Binance Ask is cheaper than Bybit Bid)
    int64_t buy_price_2 = bin_ask.getPrice().get();
    int64_t sell_price_2 = byb_bid.getPrice().get();

    double fee_2 = (buy_price_2 * binance_fee_) + (sell_price_2 * bybit_fee_);
    double net_profit_2 = (sell_price_2 - buy_price_2) - fee_2;

    if (net_profit_2 > 0) {
      int64_t trade_qty = std::min(bin_ask.getQty().get(), byb_bid.getQty().get());
      arb::log::info("[ARB DETECTED] Buy Binance / Sell Bybit | Profit Ticks: %.2f | Qty: %lld", net_profit_2, trade_qty);
      
      // Simulate Trade
      binance_.update(Side::SELL, bin_ask.getPrice(), Quantity{bin_ask.getQty().get() - trade_qty});
      bybit_.update(Side::BUY, byb_bid.getPrice(), Quantity{byb_bid.getQty().get() - trade_qty});
    }
  } // ScopedTimer ends here
  
  // Optionally, we can print the latency if an arb was found, or just print it periodically
  // arb::log::info("Strategy Eval Latency: %s", arb::nanos_to_string(eval_latency).c_str());
}

} // namespace arb