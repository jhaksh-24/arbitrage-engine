#include "arb/core/order_book.hpp"

namespace arb {
class ArbitrageStrategy {
public:
  ArbitrageStrategy(OrderBook<32, 16> &binance,
                    OrderBook<32, 16> &bybit,
                    double binance_fee = 0.0005,
                    double bybit_fee = 0.0005);

  // Called periodically (or eventually triggered by book updates)
  void evaluate() noexcept;

private:
  OrderBook<32, 16> &binance_;
  OrderBook<32, 16> &bybit_;
  double binance_fee_;
  double bybit_fee_;
};
} // namespace arb
