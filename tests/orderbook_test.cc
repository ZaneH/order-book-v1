#include "../include/orderbook.h"

#include <gtest/gtest.h>

TEST(OrderBook, AddLimit) {
  OrderBook ob = OrderBook();
  auto result = ob.add_limit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                             TimeInForce::kGoodTillCancel);

  assert(std::get<AddResultPayload>(result).status ==
         OrderStatus::kAwaitingFill);

  assert(ob.depth_at(OrderSide::kSell, Price{1}) == Quantity{0});
  assert(ob.depth_at(OrderSide::kBuy, Price{1}) == Quantity{5});
}

TEST(OrderBook, AddLimitWithBadQty) {
  OrderBook ob = OrderBook();
  auto result = ob.add_limit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{0},
                             TimeInForce::kGoodTillCancel);

  assert(std::get<RejectReason>(result) == RejectReason::kBadQty);
}

TEST(OrderBook, AddLimitWithBadPrice) {
  OrderBook ob = OrderBook();
  auto result = ob.add_limit(UserId{0}, OrderSide::kBuy, Price{0}, Quantity{5},
                             TimeInForce::kGoodTillCancel);

  assert(std::get<RejectReason>(result) == RejectReason::kBadPrice);
}
