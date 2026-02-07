#include "../include/orderbook.h"

#include <gtest/gtest.h>

TEST(OrderBook, AddLimit) {
  OrderBook ob = OrderBook();
  auto result = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                            TimeInForce::kGoodTillCancel);

  assert(std::get<AddResultPayload>(result).status ==
         OrderStatus::kAwaitingFill);

  assert(ob.DepthAt(OrderSide::kSell, Price{1}) == Quantity{0});
  assert(ob.DepthAt(OrderSide::kBuy, Price{1}) == Quantity{5});
}

TEST(OrderBook, AddLimitWithBadQty) {
  OrderBook ob = OrderBook();
  auto result = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{0},
                            TimeInForce::kGoodTillCancel);

  assert(std::get<RejectReason>(result) == RejectReason::kBadQty);
}

TEST(OrderBook, AddLimitWithBadPrice) {
  OrderBook ob = OrderBook();
  auto result = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{0}, Quantity{5},
                            TimeInForce::kGoodTillCancel);

  assert(std::get<RejectReason>(result) == RejectReason::kBadPrice);
}
