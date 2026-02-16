#include "orderbook_test.h"

TEST_F(OrderBookTest, AddLimitCrossCaseImmediateFill) {
  auto result1 =
      ob_.AddLimit(UserId{0}, OrderSide::kBuy, Price{10}, Quantity{10},
                   TimeInForce::kGoodTillCancel);  // B1
  auto result2 = ob_.AddLimit(UserId{0}, OrderSide::kSell, Price{10},
                              Quantity{5}, TimeInForce::kGoodTillCancel);  // A1

  ASSERT_EQ(result1.value().status, OrderStatus::kAwaitingFill);
  ASSERT_EQ(result2.value().status, OrderStatus::kImmediateFill);

  // A1 takes 5 from B1 leaving B1 with 5 unfilled
  ASSERT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{10}), Quantity{5});
  ASSERT_EQ(ob_.DepthAt(OrderSide::kSell, Price{10}), Quantity{0});
}

TEST_F(OrderBookTest, AddLimitCrossCasePartialFill) {
  auto result1 =
      ob_.AddLimit(UserId{0}, OrderSide::kBuy, Price{10}, Quantity{10},
                   TimeInForce::kGoodTillCancel);  // B1
  auto result2 = ob_.AddLimit(UserId{0}, OrderSide::kBuy, Price{5}, Quantity{2},
                              TimeInForce::kGoodTillCancel);  // B2
  auto result3 =
      ob_.AddLimit(UserId{0}, OrderSide::kSell, Price{10}, Quantity{20},
                   TimeInForce::kGoodTillCancel);  // A1

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
  ASSERT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{10}), Quantity{0});
  ASSERT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{5}), Quantity{2});
  ASSERT_EQ(ob_.DepthAt(OrderSide::kSell, Price{10}), Quantity{10});
}

TEST_F(OrderBookTest, AddLimitCrossCaseMultipleLevels) {
  auto result1 =
      ob_.AddLimit(UserId{0}, OrderSide::kSell, Price{15}, Quantity{10},
                   TimeInForce::kGoodTillCancel);  // A1
  auto result2 = ob_.AddLimit(UserId{0}, OrderSide::kSell, Price{10},
                              Quantity{5}, TimeInForce::kGoodTillCancel);  // A2
  auto result3 =
      ob_.AddLimit(UserId{0}, OrderSide::kBuy, Price{20}, Quantity{10},
                   TimeInForce::kGoodTillCancel);  // B1

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
  ASSERT_EQ(ob_.DepthAt(OrderSide::kSell, Price{15}), Quantity{5});
  ASSERT_EQ(ob_.DepthAt(OrderSide::kSell, Price{10}), Quantity{0});
  ASSERT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{20}), Quantity{0});
}

TEST_F(OrderBookTest, AddMarketSingleSellImmediateFill) {
  auto result1 =
      ob_.AddLimit(UserId{0}, OrderSide::kBuy, Price{10}, Quantity{10},
                   TimeInForce::kGoodTillCancel);  // B1

  ASSERT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{10}), Quantity{10});

  auto result2 = ob_.AddMarket(UserId{0}, OrderSide::kSell, Quantity{5});  // A1

  ASSERT_EQ(result2->remaining_qty, Quantity{0});

  ASSERT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{10}), Quantity{5});
  ASSERT_EQ(ob_.DepthAt(OrderSide::kSell, Price{10}), Quantity{0});
}

TEST_F(OrderBookTest, AddMarketSingleSellDiscardUnfilled) {
  auto result1 =
      ob_.AddLimit(UserId{0}, OrderSide::kBuy, Price{10}, Quantity{10},
                   TimeInForce::kGoodTillCancel);  // B1

  ASSERT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{10}), Quantity{10});

  auto result2 =
      ob_.AddMarket(UserId{0}, OrderSide::kSell, Quantity{50});  // A1

  ASSERT_EQ(result2->remaining_qty, Quantity{40});

  ASSERT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{10}), Quantity{0});
  ASSERT_EQ(ob_.DepthAt(OrderSide::kSell, Price{10}), Quantity{0});
}

TEST_F(OrderBookTest, AddMarketSingleSellMultipleLevels) {
  auto result1 =
      ob_.AddLimit(UserId{0}, OrderSide::kBuy, Price{10}, Quantity{10},
                   TimeInForce::kGoodTillCancel);  // B1
  auto result2 =
      ob_.AddLimit(UserId{0}, OrderSide::kBuy, Price{8}, Quantity{10},
                   TimeInForce::kGoodTillCancel);  // B2

  ASSERT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{10}), Quantity{10});
  ASSERT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{8}), Quantity{10});

  auto result3 =
      ob_.AddMarket(UserId{0}, OrderSide::kSell, Quantity{50});  // A1

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
  ASSERT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{10}), Quantity{0});
  ASSERT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{8}), Quantity{0});
  ASSERT_EQ(ob_.DepthAt(OrderSide::kSell, Price{10}), Quantity{0});
  ASSERT_EQ(ob_.DepthAt(OrderSide::kSell, Price{8}), Quantity{0});
}

TEST_F(OrderBookTest, AddMarketSingleBuyMultipleLevels) {
  auto result1 =
      ob_.AddLimit(UserId{0}, OrderSide::kSell, Price{10}, Quantity{10},
                   TimeInForce::kGoodTillCancel);  // A1
  auto result2 =
      ob_.AddLimit(UserId{0}, OrderSide::kSell, Price{8}, Quantity{10},
                   TimeInForce::kGoodTillCancel);  // A2

  ASSERT_EQ(ob_.DepthAt(OrderSide::kSell, Price{10}), Quantity{10});
  ASSERT_EQ(ob_.DepthAt(OrderSide::kSell, Price{8}), Quantity{10});

  auto result3 = ob_.AddMarket(UserId{0}, OrderSide::kBuy, Quantity{50});  // B1

  ASSERT_EQ(result3->remaining_qty, Quantity{30});

  ASSERT_EQ(ob_.DepthAt(OrderSide::kSell, Price{10}), Quantity{0});
  ASSERT_EQ(ob_.DepthAt(OrderSide::kSell, Price{8}), Quantity{0});
  ASSERT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{10}), Quantity{0});
  ASSERT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{8}), Quantity{0});
}
