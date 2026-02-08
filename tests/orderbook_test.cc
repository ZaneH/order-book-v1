#include "../include/orderbook.h"

#include <gtest/gtest.h>

TEST(OrderBook, AddLimit) {
  OrderBook ob = OrderBook();
  auto result = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                            TimeInForce::kGoodTillCancel);

  assert(result.value().status == OrderStatus::kAwaitingFill);

  assert(ob.DepthAt(OrderSide::kSell, Price{1}) == Quantity{0});
  assert(ob.DepthAt(OrderSide::kBuy, Price{1}) == Quantity{5});
}

TEST(OrderBook, AddMultipleLimitOrders) {
  OrderBook ob = OrderBook();
  auto result = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                            TimeInForce::kGoodTillCancel);
  auto result2 = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                             TimeInForce::kGoodTillCancel);
  auto result3 = ob.AddLimit(UserId{0}, OrderSide::kSell, Price{10},
                             Quantity{5}, TimeInForce::kGoodTillCancel);

  assert(result.value().status == OrderStatus::kAwaitingFill);
  assert(result2.value().status == OrderStatus::kAwaitingFill);

  assert(ob.DepthAt(OrderSide::kSell, Price{1}) == Quantity{0});
  assert(ob.DepthAt(OrderSide::kSell, Price{10}) == Quantity{5});
  assert(ob.DepthAt(OrderSide::kBuy, Price{1}) == Quantity{10});
}

TEST(OrderBook, AddLimitWithBadQty) {
  OrderBook ob = OrderBook();
  auto result = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{0},
                            TimeInForce::kGoodTillCancel);

  assert(result.error() == RejectReason::kBadQty);
}

TEST(OrderBook, AddLimitWithBadPrice) {
  OrderBook ob = OrderBook();
  auto result = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{0}, Quantity{5},
                            TimeInForce::kGoodTillCancel);

  assert(result.error() == RejectReason::kBadPrice);
}
