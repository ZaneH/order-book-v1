#include "orderbook_test.h"

TEST_F(OrderBookTest, AddLimitRestSingleBuy) {
  auto result = ob_.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                             TimeInForce::kGoodTillCancel);

  ASSERT_EQ(result.value().status, OrderStatus::kAwaitingFill);

  ASSERT_EQ(ob_.DepthAt(OrderSide::kSell, Price{1}), Quantity{0});
  ASSERT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{1}), Quantity{5});
}

TEST_F(OrderBookTest, AddLimitIncrementOrderId) {
  auto result1 = ob_.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                              TimeInForce::kGoodTillCancel);
  auto result2 = ob_.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                              TimeInForce::kGoodTillCancel);

  ASSERT_EQ(result1.value().order_id, OrderId{0});
  ASSERT_EQ(result2.value().order_id, OrderId{1});
}

TEST_F(OrderBookTest, AddLimitRestMultipleBuySell) {
  auto result1 = ob_.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                              TimeInForce::kGoodTillCancel);
  auto result2 = ob_.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                              TimeInForce::kGoodTillCancel);
  auto result3 = ob_.AddLimit(UserId{0}, OrderSide::kSell, Price{10},
                              Quantity{5}, TimeInForce::kGoodTillCancel);

  ASSERT_EQ(result1.value().status, OrderStatus::kAwaitingFill);
  ASSERT_EQ(result2.value().status, OrderStatus::kAwaitingFill);
  ASSERT_EQ(result3.value().status, OrderStatus::kAwaitingFill);

  ASSERT_EQ(ob_.DepthAt(OrderSide::kSell, Price{1}), Quantity{0});
  ASSERT_EQ(ob_.DepthAt(OrderSide::kSell, Price{10}), Quantity{5});
  ASSERT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{1}), Quantity{10});
}
