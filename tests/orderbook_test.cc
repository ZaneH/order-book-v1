#include "../include/orderbook.h"

#include <gtest/gtest.h>

/* NON-CROSSING TESTS */

TEST(OrderBook, AddLimitSingleBuy) {
  OrderBook ob = OrderBook();
  auto result = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                            TimeInForce::kGoodTillCancel);

  assert(result.value().status == OrderStatus::kAwaitingFill);

  assert(ob.DepthAt(OrderSide::kSell, Price{1}) == Quantity{0});
  assert(ob.DepthAt(OrderSide::kBuy, Price{1}) == Quantity{5});
}

TEST(OrderBook, AddLimitOrderNonce) {
  OrderBook ob = OrderBook();
  auto result1 = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                             TimeInForce::kGoodTillCancel);
  auto result2 = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                             TimeInForce::kGoodTillCancel);

  assert(result1.value().order_id.v == 0);
  assert(result2.value().order_id.v == 1);
}

TEST(OrderBook, AddLimitMultipleOrders) {
  OrderBook ob = OrderBook();
  auto result1 = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                             TimeInForce::kGoodTillCancel);
  auto result2 = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                             TimeInForce::kGoodTillCancel);
  auto result3 = ob.AddLimit(UserId{0}, OrderSide::kSell, Price{10},
                             Quantity{5}, TimeInForce::kGoodTillCancel);

  assert(result1.value().status == OrderStatus::kAwaitingFill);
  assert(result2.value().status == OrderStatus::kAwaitingFill);
  assert(result3.value().status == OrderStatus::kAwaitingFill);

  assert(ob.DepthAt(OrderSide::kSell, Price{1}) == Quantity{0});
  assert(ob.DepthAt(OrderSide::kSell, Price{10}) == Quantity{5});
  assert(ob.DepthAt(OrderSide::kBuy, Price{1}) == Quantity{10});
}

TEST(OrderBook, AddLimitWithBadQty) {
  OrderBook ob = OrderBook();
  auto result = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{0},
                            TimeInForce::kGoodTillCancel);

  assert(result.error() == RejectReason::kBadQty);
  assert(ob.DepthAt(OrderSide::kBuy, Price{1}) == Quantity{0});
}

TEST(OrderBook, AddLimitWithBadPrice) {
  OrderBook ob = OrderBook();
  auto result = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{0}, Quantity{5},
                            TimeInForce::kGoodTillCancel);

  assert(result.error() == RejectReason::kBadPrice);
  assert(ob.DepthAt(OrderSide::kBuy, Price{0}) == Quantity{0});
}

TEST(OrderBook, CancelRestingOrder) {
  OrderBook ob = OrderBook();
  auto result1 = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                             TimeInForce::kGoodTillCancel);
  auto result2 = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                             TimeInForce::kGoodTillCancel);
  auto result3 = ob.AddLimit(UserId{0}, OrderSide::kSell, Price{10},
                             Quantity{5}, TimeInForce::kGoodTillCancel);

  bool cancel_existing = ob.Cancel(result2.value().order_id);
  assert(cancel_existing == true);
  bool cancel_nonexisting = ob.Cancel(OrderId{999});
  assert(cancel_nonexisting == false);

  assert(result1.value().status == OrderStatus::kAwaitingFill);
  assert(result2.value().status == OrderStatus::kAwaitingFill);
  assert(result3.value().status == OrderStatus::kAwaitingFill);

  assert(ob.DepthAt(OrderSide::kSell, Price{10}) == Quantity{5});
  assert(ob.DepthAt(OrderSide::kBuy, Price{1}) == Quantity{5});
}

/* CROSSING TESTS */

TEST(OrderBook, CancelFullyTradedOrderFails) {
  OrderBook ob = OrderBook();
  auto result1 = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{10},
                             Quantity{10}, TimeInForce::kGoodTillCancel);  // B1
  auto result2 = ob.AddLimit(UserId{0}, OrderSide::kSell, Price{10},
                             Quantity{5}, TimeInForce::kGoodTillCancel);  // A1
  auto result3 = ob.AddLimit(UserId{0}, OrderSide::kSell, Price{10},
                             Quantity{5}, TimeInForce::kGoodTillCancel);  // A2

  assert(result1.value().status == OrderStatus::kAwaitingFill);
  assert(result2.value().status == OrderStatus::kImmediateFill);

  // A1 takes 5 from B1, immediately filling A1. A2 then takes 5 from B1,
  // resulting in A2 being immediately filled and removing B1 from the book.
  assert(ob.DepthAt(OrderSide::kBuy, Price{10}) == Quantity{0});
  assert(ob.DepthAt(OrderSide::kSell, Price{10}) == Quantity{0});

  assert(ob.Cancel(result1->order_id) == false);
}

TEST(OrderBook, AddLimitCrossCaseImmediateFill) {
  OrderBook ob = OrderBook();
  auto result1 = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{10},
                             Quantity{10}, TimeInForce::kGoodTillCancel);  // B1
  auto result2 = ob.AddLimit(UserId{0}, OrderSide::kSell, Price{10},
                             Quantity{5}, TimeInForce::kGoodTillCancel);  // A1

  assert(result1.value().status == OrderStatus::kAwaitingFill);
  assert(result2.value().status == OrderStatus::kImmediateFill);

  // A1 takes 5 from B1 leaving B1 with 5 unfilled
  assert(ob.DepthAt(OrderSide::kBuy, Price{10}) == Quantity{5});
  assert(ob.DepthAt(OrderSide::kSell, Price{10}) == Quantity{0});
}

TEST(OrderBook, AddLimitCrossCasePartialFill) {
  OrderBook ob = OrderBook();
  auto result1 = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{10},
                             Quantity{10}, TimeInForce::kGoodTillCancel);  // B1
  auto result2 = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{5}, Quantity{2},
                             TimeInForce::kGoodTillCancel);  // B2
  auto result3 = ob.AddLimit(UserId{0}, OrderSide::kSell, Price{10},
                             Quantity{20}, TimeInForce::kGoodTillCancel);  // A1

  /*
   * Incoming: A1 wants to sell 20 @ 10
   *
   * [bids]
   * 10: B1(10)
   * 5:  B2(2)
   */

  assert(result1.value().status == OrderStatus::kAwaitingFill);
  assert(result2.value().status == OrderStatus::kAwaitingFill);
  assert(result3.value().status == OrderStatus::kPartialFill);

  // A1 takes 10 from B1, resulting in B1 being removed from the book and
  // leaving A1 with 10 unfilled. B2 is untouched, resting in the book.
  assert(ob.DepthAt(OrderSide::kBuy, Price{10}) == Quantity{0});
  assert(ob.DepthAt(OrderSide::kBuy, Price{5}) == Quantity{2});
  assert(ob.DepthAt(OrderSide::kSell, Price{10}) == Quantity{10});
}

TEST(OrderBook, AddLimitCrossCaseMultipleLevels) {
  OrderBook ob = OrderBook();
  auto result1 =
      ob.AddLimit(UserId{0}, OrderSide::kSell, Price{15}, Quantity{10},
                  TimeInForce::kGoodTillCancel);  // A1
  auto result2 = ob.AddLimit(UserId{0}, OrderSide::kSell, Price{10},
                             Quantity{5}, TimeInForce::kGoodTillCancel);  // A2
  auto result3 = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{20},
                             Quantity{10}, TimeInForce::kGoodTillCancel);  // B1

  /*
   * Incoming: B1 wants to buy 10 @ 20
   *
   * [asks]
   * 15: A1(10)
   * 10: A2(5)
   */

  assert(result1.value().status == OrderStatus::kAwaitingFill);
  assert(result2.value().status == OrderStatus::kAwaitingFill);
  assert(result3.value().status == OrderStatus::kImmediateFill);

  // B1 takes 5 from A2, resulting in A2 being removed from the book. B1 takes 5
  // from A1 and filling B1. A1 is left with 5 unfilled.
  assert(ob.DepthAt(OrderSide::kSell, Price{15}) == Quantity{5});
  assert(ob.DepthAt(OrderSide::kSell, Price{10}) == Quantity{0});
  assert(ob.DepthAt(OrderSide::kBuy, Price{20}) == Quantity{0});
}
