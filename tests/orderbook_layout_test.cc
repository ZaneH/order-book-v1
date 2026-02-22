#include "orderbook_test.h"

namespace order_book_v1 {
TEST_F(OrderBookTest, AddLimitRestSingleBuy) {
  // Arrange
  auto result = AddLimitOk(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                           TimeInForce::kGoodTillCancel);

  // Assert
  EXPECT_EQ(result->status, OrderStatus::kAwaitingFill);
  EXPECT_EQ(ob_.DepthAt(OrderSide::kSell, Price{1}), Quantity{0});
  EXPECT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{1}), Quantity{5});
}

TEST_F(OrderBookTest, AddLimitIncrementOrderId) {
  // Arrange
  auto result1 = AddLimitOk(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                            TimeInForce::kGoodTillCancel);
  auto result2 = AddLimitOk(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                            TimeInForce::kGoodTillCancel);

  // Assert
  EXPECT_EQ(result1->order_id, OrderId{1});
  EXPECT_EQ(result2->order_id, OrderId{2});
}

TEST_F(OrderBookTest, AddLimitRestMultipleBuySell) {
  // Arrange
  ArrangeBidLevels({{Price{1}, Quantity{5}}, {Price{1}, Quantity{5}}});
  ArrangeAskLevels({{Price{10}, Quantity{5}}});

  // Assert
  EXPECT_EQ(ob_.DepthAt(OrderSide::kSell, Price{1}), Quantity{0});
  EXPECT_EQ(ob_.DepthAt(OrderSide::kSell, Price{10}), Quantity{5});
  EXPECT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{1}), Quantity{10});
}
}  // namespace order_book_v1
