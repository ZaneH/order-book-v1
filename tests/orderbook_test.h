#ifndef TESTS_ORDERBOOK_TEST_H_
#define TESTS_ORDERBOOK_TEST_H_

#include <gtest/gtest.h>
#include <orderbook.h>

#include <initializer_list>
#include <utility>

#include "types.h"

namespace order_book_v1 {
using LevelSpec = std::pair<Price, Quantity>;

class OrderBookTest : public testing::Test {
 protected:
  OrderBook ob_;

  std::vector<OrderId> ArrangeBidLevels(
      std::initializer_list<LevelSpec> levels);
  std::vector<OrderId> ArrangeAskLevels(
      std::initializer_list<LevelSpec> levels);

  void AssertAddResult(const AddResult& result, OrderStatus status,
                       Quantity remaining, size_t n_trades);
  AddResult AddLimitOk(UserId user_id, OrderSide side, Price price,
                       Quantity qty, TimeInForce tif);
};
}  // namespace order_book_v1

#endif
