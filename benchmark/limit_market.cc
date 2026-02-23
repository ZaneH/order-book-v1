#include <benchmark/benchmark.h>

#include <ostream>
#include <random>
#include <sstream>

#include "orderbook.h"
#include "types.h"

namespace order_book_v1 {
struct BulkOrders {
  std::vector<UserId> user_ids;
  std::vector<OrderSide> sides;
  std::vector<Price> prices;
  std::vector<Quantity> qtys;
  std::vector<TimeInForce> tifs;
};

static BulkOrders PrepareOrders() {
  std::mt19937 rng(1337);
  std::uniform_int_distribution<std::mt19937::result_type> user_id_rn(0, 100);
  std::uniform_int_distribution<std::mt19937::result_type> sides_rn(0, 1);
  std::uniform_int_distribution<std::mt19937::result_type> price_rn(1, 10);
  std::uniform_int_distribution<std::mt19937::result_type> qty_rn(1, 10);
  std::uniform_int_distribution<std::mt19937::result_type> tif_rn(0, 1);

  std::vector<UserId> user_ids;
  std::vector<OrderSide> sides;
  std::vector<Price> prices;
  std::vector<Quantity> qtys;
  std::vector<TimeInForce> tifs;
  std::vector<EventType> types;

  for (auto i = 0; i < 1000; i++) {
    Underlying user_id = user_id_rn(rng);
    OrderSide side = sides_rn(rng) == 0 ? OrderSide::kBuy : OrderSide::kSell;
    Underlying price = price_rn(rng);
    Underlying qty = qty_rn(rng);
    TimeInForce tif = tif_rn(rng) == 0 ? TimeInForce::kGoodTillCancel
                                       : TimeInForce::kImmediateOrCancel;

    user_ids.push_back(UserId{user_id});
    sides.push_back(side);
    prices.push_back(Price{price});
    qtys.push_back(Quantity{qty});
    tifs.push_back(tif);
  }

  return BulkOrders{
      user_ids, sides, prices, qtys, tifs,
  };
}

static OrderBook PrepareOrderBook(std::ostream& buf) {
  OrderBook ob(&buf);

  BulkOrders orders = PrepareOrders();
  for (size_t i = 0; i < orders.user_ids.size(); i++) {
    auto result = ob.AddLimit(orders.user_ids[i], orders.sides[i],
                              orders.prices[i], orders.qtys[i], orders.tifs[i]);
  }
  return ob;
}

class PreppedOrdersFixture : public benchmark::Fixture {
 public:
  void SetUp(const benchmark::State&) override { orders_ = PrepareOrders(); }
  void TearDown(const benchmark::State&) override {}

  BulkOrders orders_;
};

class PreppedOrderBookFixture : public benchmark::Fixture {
 public:
  void SetUp(const benchmark::State&) override {
    ob_ = PrepareOrderBook(buf);
    orders_ = PrepareOrders();
  }
  void TearDown(const benchmark::State&) override {}

  std::ostringstream buf;
  BulkOrders orders_;
  OrderBook ob_;
};

BENCHMARK_DEFINE_F(PreppedOrdersFixture, AddLimit)(benchmark::State& st) {
  std::ostringstream buf{};
  OrderBook ob{&buf};

  size_t i = 0;
  for (auto _ : st) {
    const size_t idx = i++ % orders_.user_ids.size();
    auto result =
        ob.AddLimit(orders_.user_ids[idx], orders_.sides[idx],
                    orders_.prices[idx], orders_.qtys[idx], orders_.tifs[idx]);
    benchmark::DoNotOptimize(result);
  }
}

BENCHMARK_DEFINE_F(PreppedOrderBookFixture, AddMarket)(benchmark::State& st) {
  size_t i = 0;
  for (auto _ : st) {
    const size_t idx = i++ % orders_.user_ids.size();
    auto result = ob_.AddMarket(orders_.user_ids[idx], orders_.sides[idx],
                                orders_.qtys[idx]);
    benchmark::DoNotOptimize(result);
  }
}

BENCHMARK_REGISTER_F(PreppedOrdersFixture, AddLimit);
BENCHMARK_REGISTER_F(PreppedOrderBookFixture, AddMarket);
}  // namespace order_book_v1
