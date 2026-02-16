#include "orderbook_test.h"

TEST_F(OrderBookTest, CancelRestingOrder) {
  auto result1 = ob_.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                              TimeInForce::kGoodTillCancel);
  auto result2 = ob_.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                              TimeInForce::kGoodTillCancel);
  auto result3 = ob_.AddLimit(UserId{0}, OrderSide::kSell, Price{10},
                              Quantity{5}, TimeInForce::kGoodTillCancel);

  bool cancel_result = ob_.Cancel(result2.value().order_id);
  ASSERT_TRUE(cancel_result);

  ASSERT_EQ(result1.value().status, OrderStatus::kAwaitingFill);
  ASSERT_EQ(result2.value().status, OrderStatus::kAwaitingFill);
  ASSERT_EQ(result3.value().status, OrderStatus::kAwaitingFill);

  ASSERT_EQ(ob_.DepthAt(OrderSide::kSell, Price{10}), Quantity{5});
  ASSERT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{1}), Quantity{5});
}

TEST_F(OrderBookTest, CancelNonexistentOrder) {
  bool cancel_result = ob_.Cancel(OrderId{999});
  ASSERT_FALSE(cancel_result);
}

TEST_F(OrderBookTest, CancelOnlyOrder) {
  auto result1 = ob_.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                              TimeInForce::kGoodTillCancel);
  ob_.Cancel(result1->order_id);
  ASSERT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{1}), Quantity{0});
}

TEST_F(OrderBookTest, CancelFullyTradedOrderFails) {
  auto result1 =
      ob_.AddLimit(UserId{0}, OrderSide::kBuy, Price{10}, Quantity{10},
                   TimeInForce::kGoodTillCancel);  // B1
  auto result2 = ob_.AddLimit(UserId{0}, OrderSide::kSell, Price{10},
                              Quantity{5}, TimeInForce::kGoodTillCancel);  // A1
  auto result3 = ob_.AddLimit(UserId{0}, OrderSide::kSell, Price{10},
                              Quantity{5}, TimeInForce::kGoodTillCancel);  // A2

  ASSERT_EQ(result1.value().status, OrderStatus::kAwaitingFill);
  ASSERT_EQ(result2.value().status, OrderStatus::kImmediateFill);

  // A1 takes 5 from B1, immediately filling A1. A2 then takes 5 from B1,
  // resulting in A2 being immediately filled and removing B1 from the book.
  ASSERT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{10}), Quantity{0});
  ASSERT_EQ(ob_.DepthAt(OrderSide::kSell, Price{10}), Quantity{0});

  ASSERT_FALSE(ob_.Cancel(result1->order_id));
}
