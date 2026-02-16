#include "orderbook_test.h"

#include <cstddef>
#include <vector>

std::vector<OrderId> OrderBookTest::ArrangeBidLevels(
    std::initializer_list<LevelSpec> levels) {
  std::vector<OrderId> ids{};
  for (const auto& [price, qty] : levels) {
    auto r = AddLimitOk(UserId{0}, OrderSide::kBuy, price, qty,
                        TimeInForce::kGoodTillCancel);
    ids.emplace_back(r->order_id);
  }

  return ids;
}

std::vector<OrderId> OrderBookTest::ArrangeAskLevels(
    std::initializer_list<LevelSpec> levels) {
  std::vector<OrderId> ids{};
  for (const auto& [price, qty] : levels) {
    auto r = AddLimitOk(UserId{0}, OrderSide::kSell, price, qty,
                        TimeInForce::kGoodTillCancel);
    ids.emplace_back(r->order_id);
  }

  return ids;
}

AddResult OrderBookTest::AddLimitOk(UserId user_id, OrderSide side, Price price,
                                    Quantity qty, TimeInForce tif) {
  AddResult result = ob_.AddLimit(user_id, side, price, qty, tif);
  EXPECT_TRUE(result.has_value());
  return result;
}

void OrderBookTest::AssertAddResult(const AddResult& result, OrderStatus status,
                                    Quantity remaining, size_t n_trades) {
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->status, status);
  EXPECT_EQ(result->remaining_qty, remaining);
  EXPECT_EQ(result->immediate_trades.size(), n_trades);
}
