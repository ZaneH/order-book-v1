#include "../include/orderbook.h"

#include <gtest/gtest.h>

/* NON-CROSSING TESTS */

TEST(OrderBook, AddLimitSingleBuy) {
  OrderBook ob = OrderBook();
  auto result = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                            TimeInForce::kGoodTillCancel);

  ASSERT_EQ(result.value().status, OrderStatus::kAwaitingFill);

  ASSERT_EQ(ob.DepthAt(OrderSide::kSell, Price{1}), Quantity{0});
  ASSERT_EQ(ob.DepthAt(OrderSide::kBuy, Price{1}), Quantity{5});
}

TEST(OrderBook, AddLimitIncrementOrderId) {
  OrderBook ob = OrderBook();
  auto result1 = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                             TimeInForce::kGoodTillCancel);
  auto result2 = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                             TimeInForce::kGoodTillCancel);

  ASSERT_EQ(result1.value().order_id, OrderId{0});
  ASSERT_EQ(result2.value().order_id, OrderId{1});
}

TEST(OrderBook, AddLimitMultipleOrders) {
  OrderBook ob = OrderBook();
  auto result1 = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                             TimeInForce::kGoodTillCancel);
  auto result2 = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                             TimeInForce::kGoodTillCancel);
  auto result3 = ob.AddLimit(UserId{0}, OrderSide::kSell, Price{10},
                             Quantity{5}, TimeInForce::kGoodTillCancel);

  ASSERT_EQ(result1.value().status, OrderStatus::kAwaitingFill);
  ASSERT_EQ(result2.value().status, OrderStatus::kAwaitingFill);
  ASSERT_EQ(result3.value().status, OrderStatus::kAwaitingFill);

  ASSERT_EQ(ob.DepthAt(OrderSide::kSell, Price{1}), Quantity{0});
  ASSERT_EQ(ob.DepthAt(OrderSide::kSell, Price{10}), Quantity{5});
  ASSERT_EQ(ob.DepthAt(OrderSide::kBuy, Price{1}), Quantity{10});
}

TEST(OrderBook, AddLimitWithBadQty) {
  OrderBook ob = OrderBook();
  auto result = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{0},
                            TimeInForce::kGoodTillCancel);

  ASSERT_EQ(result.error(), RejectReason::kBadQty);
  ASSERT_EQ(ob.DepthAt(OrderSide::kBuy, Price{1}), Quantity{0});
}

TEST(OrderBook, AddLimitWithBadPrice) {
  OrderBook ob = OrderBook();
  auto result = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{0}, Quantity{5},
                            TimeInForce::kGoodTillCancel);

  ASSERT_EQ(result.error(), RejectReason::kBadPrice);
  ASSERT_EQ(ob.DepthAt(OrderSide::kBuy, Price{0}), Quantity{0});
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
  ASSERT_TRUE(cancel_existing);
  bool cancel_nonexisting = ob.Cancel(OrderId{999});
  ASSERT_FALSE(cancel_nonexisting);

  ASSERT_EQ(result1.value().status, OrderStatus::kAwaitingFill);
  ASSERT_EQ(result2.value().status, OrderStatus::kAwaitingFill);
  ASSERT_EQ(result3.value().status, OrderStatus::kAwaitingFill);

  ASSERT_EQ(ob.DepthAt(OrderSide::kSell, Price{10}), Quantity{5});
  ASSERT_EQ(ob.DepthAt(OrderSide::kBuy, Price{1}), Quantity{5});
}

TEST(OrderBook, CancelOnlyOrder) {
  OrderBook ob = OrderBook();
  auto result1 = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                             TimeInForce::kGoodTillCancel);
  ob.Cancel(result1->order_id);
  ASSERT_EQ(ob.DepthAt(OrderSide::kBuy, Price{1}), Quantity{0});
}

TEST(OrderBook, AddMarketEmptyBook) {
  OrderBook ob = OrderBook();

  auto result1 = ob.AddMarket(UserId{0}, OrderSide::kSell, Quantity{5});
  auto result2 = ob.AddMarket(UserId{0}, OrderSide::kBuy, Quantity{5});

  ASSERT_EQ(result1.error(), RejectReason::kEmptyBookForMarket);
  ASSERT_EQ(result2.error(), RejectReason::kEmptyBookForMarket);
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

  ASSERT_EQ(result1.value().status, OrderStatus::kAwaitingFill);
  ASSERT_EQ(result2.value().status, OrderStatus::kImmediateFill);

  // A1 takes 5 from B1, immediately filling A1. A2 then takes 5 from B1,
  // resulting in A2 being immediately filled and removing B1 from the book.
  ASSERT_EQ(ob.DepthAt(OrderSide::kBuy, Price{10}), Quantity{0});
  ASSERT_EQ(ob.DepthAt(OrderSide::kSell, Price{10}), Quantity{0});

  ASSERT_FALSE(ob.Cancel(result1->order_id));
}

TEST(OrderBook, AddLimitCrossCaseImmediateFill) {
  OrderBook ob = OrderBook();
  auto result1 = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{10},
                             Quantity{10}, TimeInForce::kGoodTillCancel);  // B1
  auto result2 = ob.AddLimit(UserId{0}, OrderSide::kSell, Price{10},
                             Quantity{5}, TimeInForce::kGoodTillCancel);  // A1

  ASSERT_EQ(result1.value().status, OrderStatus::kAwaitingFill);
  ASSERT_EQ(result2.value().status, OrderStatus::kImmediateFill);

  // A1 takes 5 from B1 leaving B1 with 5 unfilled
  ASSERT_EQ(ob.DepthAt(OrderSide::kBuy, Price{10}), Quantity{5});
  ASSERT_EQ(ob.DepthAt(OrderSide::kSell, Price{10}), Quantity{0});
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

  ASSERT_EQ(result1.value().status, OrderStatus::kAwaitingFill);
  ASSERT_EQ(result2.value().status, OrderStatus::kAwaitingFill);
  ASSERT_EQ(result3.value().status, OrderStatus::kPartialFill);

  // A1 takes 10 from B1, resulting in B1 being removed from the book and
  // leaving A1 with 10 unfilled. B2 is untouched, resting in the book.
  ASSERT_EQ(ob.DepthAt(OrderSide::kBuy, Price{10}), Quantity{0});
  ASSERT_EQ(ob.DepthAt(OrderSide::kBuy, Price{5}), Quantity{2});
  ASSERT_EQ(ob.DepthAt(OrderSide::kSell, Price{10}), Quantity{10});
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
   * 10: A2(5)
   * 15: A1(10)
   */

  ASSERT_EQ(result1.value().status, OrderStatus::kAwaitingFill);
  ASSERT_EQ(result2.value().status, OrderStatus::kAwaitingFill);
  ASSERT_EQ(result3.value().status, OrderStatus::kImmediateFill);

  // B1 takes 5 from A2, resulting in A2 being removed from the book.
  // B1 takes 5 from A1, filling B1. A1 is left with 5 unfilled.
  ASSERT_EQ(ob.DepthAt(OrderSide::kSell, Price{15}), Quantity{5});
  ASSERT_EQ(ob.DepthAt(OrderSide::kSell, Price{10}), Quantity{0});
  ASSERT_EQ(ob.DepthAt(OrderSide::kBuy, Price{20}), Quantity{0});
}

TEST(OrderBook, AddMarketSingleSellImmediateFill) {
  OrderBook ob = OrderBook();
  auto result1 = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{10},
                             Quantity{10}, TimeInForce::kGoodTillCancel);  // B1

  ASSERT_EQ(ob.DepthAt(OrderSide::kBuy, Price{10}), Quantity{10});

  auto result2 = ob.AddMarket(UserId{0}, OrderSide::kSell, Quantity{5});  // A1

  ASSERT_EQ(result2->remaining_qty, Quantity{0});

  ASSERT_EQ(ob.DepthAt(OrderSide::kBuy, Price{10}), Quantity{5});
  ASSERT_EQ(ob.DepthAt(OrderSide::kSell, Price{10}), Quantity{0});
}

TEST(OrderBook, AddMarketSingleSellDiscardUnfilled) {
  OrderBook ob = OrderBook();
  auto result1 = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{10},
                             Quantity{10}, TimeInForce::kGoodTillCancel);  // B1

  ASSERT_EQ(ob.DepthAt(OrderSide::kBuy, Price{10}), Quantity{10});

  auto result2 = ob.AddMarket(UserId{0}, OrderSide::kSell, Quantity{50});  // A1

  ASSERT_EQ(result2->remaining_qty, Quantity{40});

  ASSERT_EQ(ob.DepthAt(OrderSide::kBuy, Price{10}), Quantity{0});
  ASSERT_EQ(ob.DepthAt(OrderSide::kSell, Price{10}), Quantity{0});
}

TEST(OrderBook, AddMarketSingleSellMultipleLevels) {
  OrderBook ob = OrderBook();
  auto result1 = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{10},
                             Quantity{10}, TimeInForce::kGoodTillCancel);  // B1
  auto result2 = ob.AddLimit(UserId{0}, OrderSide::kBuy, Price{8}, Quantity{10},
                             TimeInForce::kGoodTillCancel);  // B2

  ASSERT_EQ(ob.DepthAt(OrderSide::kBuy, Price{10}), Quantity{10});
  ASSERT_EQ(ob.DepthAt(OrderSide::kBuy, Price{8}), Quantity{10});

  auto result3 = ob.AddMarket(UserId{0}, OrderSide::kSell, Quantity{50});  // A1

  /*
   * Incoming: A1 wants to sell 50 @ Market
   *
   * [bids]
   * 10: B1(10)
   * 8:  B2(10)
   */

  ASSERT_EQ(result3->remaining_qty, Quantity{30});

  // A1 takes 10 from B1, resulting in B1 being removed from the book.
  // A1 takes 10 from B2, resulting in B2 being removed from the book.
  // A1 is left with 30 unfilled and the remaining is discarded.
  ASSERT_EQ(ob.DepthAt(OrderSide::kBuy, Price{10}), Quantity{0});
  ASSERT_EQ(ob.DepthAt(OrderSide::kBuy, Price{8}), Quantity{0});
  ASSERT_EQ(ob.DepthAt(OrderSide::kSell, Price{10}), Quantity{0});
  ASSERT_EQ(ob.DepthAt(OrderSide::kSell, Price{8}), Quantity{0});
}

TEST(OrderBook, AddMarketSingleBuyMultipleLevels) {
  OrderBook ob = OrderBook();
  auto result1 = ob.AddLimit(UserId{0}, OrderSide::kSell, Price{10},
                             Quantity{10}, TimeInForce::kGoodTillCancel);  // A1
  auto result2 = ob.AddLimit(UserId{0}, OrderSide::kSell, Price{8},
                             Quantity{10}, TimeInForce::kGoodTillCancel);  // A2

  ASSERT_EQ(ob.DepthAt(OrderSide::kSell, Price{10}), Quantity{10});
  ASSERT_EQ(ob.DepthAt(OrderSide::kSell, Price{8}), Quantity{10});

  auto result3 = ob.AddMarket(UserId{0}, OrderSide::kBuy, Quantity{50});  // B1

  ASSERT_EQ(result3->remaining_qty, Quantity{30});

  ASSERT_EQ(ob.DepthAt(OrderSide::kSell, Price{10}), Quantity{0});
  ASSERT_EQ(ob.DepthAt(OrderSide::kSell, Price{8}), Quantity{0});
  ASSERT_EQ(ob.DepthAt(OrderSide::kBuy, Price{10}), Quantity{0});
  ASSERT_EQ(ob.DepthAt(OrderSide::kBuy, Price{8}), Quantity{0});
}
