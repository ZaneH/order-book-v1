#include <benchmark/benchmark.h>

#include <cstddef>
#include <ostream>
#include <streambuf>
#include <vector>

#include "orderbook.h"
#include "types.h"

namespace order_book_v1 {
namespace {
constexpr Price kMid{100};
constexpr Quantity kLevelQty{10};
constexpr TimeInForce kGtc = TimeInForce::kGoodTillCancel;

class NullBuffer : public std::streambuf {
 protected:
  int overflow(int c) override { return traits_type::not_eof(c); }
};

class NullStream : public std::ostream {
 public:
  NullStream() : std::ostream(&buffer_) {}

 private:
  NullBuffer buffer_;
};

void SeedOpposingBook(OrderBook& ob, OrderSide taker_side, std::size_t levels,
                      std::size_t orders_per_level, Quantity qty_per_order) {
  const OrderSide maker_side =
      taker_side == OrderSide::kBuy ? OrderSide::kSell : OrderSide::kBuy;

  for (std::size_t level = 0; level < levels; ++level) {
    Price level_price =
        maker_side == OrderSide::kSell
            ? Price{static_cast<Underlying>(kMid.v + 1 + level)}
            : Price{static_cast<Underlying>(kMid.v - 1 - level)};
    for (std::size_t i = 0; i < orders_per_level; ++i) {
      auto add =
          ob.AddLimit(UserId{static_cast<Underlying>(1000 + level * 100 + i)},
                      maker_side, level_price, qty_per_order, kGtc);
      benchmark::DoNotOptimize(add);
    }
  }
}

std::vector<OrderId> SeedCancelableOrders(OrderBook& ob, std::size_t levels,
                                          std::size_t orders_per_level) {
  std::vector<OrderId> ids;
  ids.reserve(levels * orders_per_level);

  for (std::size_t level = 0; level < levels; ++level) {
    Price px{static_cast<Underlying>(kMid.v - 5 - level)};
    for (std::size_t i = 0; i < orders_per_level; ++i) {
      auto add =
          ob.AddLimit(UserId{static_cast<Underlying>(2000 + level * 100 + i)},
                      OrderSide::kBuy, px, kLevelQty, kGtc);
      if (add.has_value()) ids.push_back(add->order_id);
    }
  }
  return ids;
}
}  // namespace

static void BM_AddLimit_Resting(benchmark::State& st) {
  NullStream sink;
  std::size_t total_rejects = 0;
  std::size_t total_trades = 0;

  for (auto _ : st) {
    st.PauseTiming();
    OrderBook ob(&sink);
    SeedOpposingBook(ob, OrderSide::kBuy, static_cast<std::size_t>(st.range(0)),
                     static_cast<std::size_t>(st.range(1)), kLevelQty);
    st.ResumeTiming();

    auto add = ob.AddLimit(UserId{1}, OrderSide::kBuy, Price{kMid.v - 1},
                           Quantity{5}, kGtc);
    benchmark::DoNotOptimize(add);

    if (add.has_value()) {
      total_trades += add->immediate_trades.size();
    } else {
      ++total_rejects;
    }
  }

  st.counters["trades_per_op"] = benchmark::Counter(
      static_cast<double>(total_trades), benchmark::Counter::kAvgIterations);
  st.counters["reject_rate"] = benchmark::Counter(
      static_cast<double>(total_rejects), benchmark::Counter::kAvgIterations);
}

static void BM_AddLimit_CrossingImmediateFill(benchmark::State& st) {
  NullStream sink;
  std::size_t total_trades = 0;

  for (auto _ : st) {
    st.PauseTiming();
    OrderBook ob(&sink);
    SeedOpposingBook(ob, OrderSide::kBuy, static_cast<std::size_t>(st.range(0)),
                     static_cast<std::size_t>(st.range(1)), kLevelQty);
    st.ResumeTiming();

    auto add = ob.AddLimit(UserId{2}, OrderSide::kBuy, Price{kMid.v + 10},
                           Quantity{5}, kGtc);
    benchmark::DoNotOptimize(add);
    if (add.has_value()) total_trades += add->immediate_trades.size();
  }

  st.counters["trades_per_op"] = benchmark::Counter(
      static_cast<double>(total_trades), benchmark::Counter::kAvgIterations);
}

static void BM_AddMarket_FullFill(benchmark::State& st) {
  NullStream sink;
  std::size_t total_trades = 0;
  std::size_t total_rejects = 0;

  for (auto _ : st) {
    st.PauseTiming();
    OrderBook ob(&sink);
    SeedOpposingBook(ob, OrderSide::kBuy, static_cast<std::size_t>(st.range(0)),
                     static_cast<std::size_t>(st.range(1)), kLevelQty);
    st.ResumeTiming();

    auto add = ob.AddMarket(UserId{3}, OrderSide::kBuy, Quantity{5});
    benchmark::DoNotOptimize(add);
    if (add.has_value()) {
      total_trades += add->immediate_trades.size();
    } else {
      ++total_rejects;
    }
  }

  st.counters["trades_per_op"] = benchmark::Counter(
      static_cast<double>(total_trades), benchmark::Counter::kAvgIterations);
  st.counters["reject_rate"] = benchmark::Counter(
      static_cast<double>(total_rejects), benchmark::Counter::kAvgIterations);
}

static void BM_AddMarket_PartialFill(benchmark::State& st) {
  NullStream sink;
  std::size_t total_trades = 0;
  std::size_t total_remaining = 0;

  for (auto _ : st) {
    st.PauseTiming();
    OrderBook ob(&sink);
    SeedOpposingBook(ob, OrderSide::kBuy, 1, 1, Quantity{3});
    st.ResumeTiming();

    auto add = ob.AddMarket(UserId{4}, OrderSide::kBuy, Quantity{9});
    benchmark::DoNotOptimize(add);
    if (add.has_value()) {
      total_trades += add->immediate_trades.size();
      total_remaining += add->remaining_qty.v;
    }
  }

  st.counters["trades_per_op"] = benchmark::Counter(
      static_cast<double>(total_trades), benchmark::Counter::kAvgIterations);
  st.counters["remaining_qty_per_op"] = benchmark::Counter(
      static_cast<double>(total_remaining), benchmark::Counter::kAvgIterations);
}

static void BM_AddMarket_EmptyReject(benchmark::State& st) {
  NullStream sink;
  std::size_t total_rejects = 0;

  for (auto _ : st) {
    st.PauseTiming();
    OrderBook ob(&sink);
    st.ResumeTiming();

    auto add = ob.AddMarket(UserId{5}, OrderSide::kBuy, Quantity{1});
    benchmark::DoNotOptimize(add);
    if (!add.has_value()) ++total_rejects;
  }

  st.counters["reject_rate"] = benchmark::Counter(
      static_cast<double>(total_rejects), benchmark::Counter::kAvgIterations);
}

static void BM_Cancel_Hit(benchmark::State& st) {
  NullStream sink;
  std::size_t total_success = 0;

  for (auto _ : st) {
    st.PauseTiming();
    OrderBook ob(&sink);
    auto ids = SeedCancelableOrders(ob, static_cast<std::size_t>(st.range(0)),
                                    static_cast<std::size_t>(st.range(1)));
    st.ResumeTiming();

    bool ok = ob.Cancel(ids.front());
    benchmark::DoNotOptimize(ok);
    if (ok) ++total_success;
  }

  st.counters["success_rate"] = benchmark::Counter(
      static_cast<double>(total_success), benchmark::Counter::kAvgIterations);
}

static void BM_Cancel_Miss(benchmark::State& st) {
  NullStream sink;
  std::size_t total_miss = 0;

  for (auto _ : st) {
    st.PauseTiming();
    OrderBook ob(&sink);
    auto ids = SeedCancelableOrders(ob, static_cast<std::size_t>(st.range(0)),
                                    static_cast<std::size_t>(st.range(1)));
    benchmark::DoNotOptimize(ids);
    st.ResumeTiming();

    bool ok = ob.Cancel(OrderId{0x7fffffff});
    benchmark::DoNotOptimize(ok);
    if (!ok) ++total_miss;
  }

  st.counters["miss_rate"] = benchmark::Counter(
      static_cast<double>(total_miss), benchmark::Counter::kAvgIterations);
}

BENCHMARK(BM_AddLimit_Resting)->Args({5, 10})->Args({20, 20});
BENCHMARK(BM_AddLimit_CrossingImmediateFill)->Args({5, 10})->Args({20, 20});
BENCHMARK(BM_AddMarket_FullFill)->Args({5, 10})->Args({20, 20});
BENCHMARK(BM_AddMarket_PartialFill);
BENCHMARK(BM_AddMarket_EmptyReject);
BENCHMARK(BM_Cancel_Hit)->Args({5, 10})->Args({20, 20});
BENCHMARK(BM_Cancel_Miss)->Args({5, 10})->Args({20, 20});
}  // namespace order_book_v1
