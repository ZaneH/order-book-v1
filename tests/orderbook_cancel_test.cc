#include "orderbook_test.h"

namespace order_book_v1 {
TEST_F(OrderBookTest, CancelRestingOrder) {
  // Arrange
  auto b1 = AddLimitOk(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                       TimeInForce::kGoodTillCancel);
  auto b2 = AddLimitOk(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                       TimeInForce::kGoodTillCancel);
  auto a1 = AddLimitOk(UserId{0}, OrderSide::kSell, Price{10}, Quantity{5},
                       TimeInForce::kGoodTillCancel);

  // Act
  bool result = ob_.Cancel(b2->order_id);

  // Assert
  EXPECT_TRUE(result);
  AssertAddResult(b1, OrderStatus::kAwaitingFill, Quantity{5}, 0);
  AssertAddResult(b2, OrderStatus::kAwaitingFill, Quantity{5}, 0);
  AssertAddResult(a1, OrderStatus::kAwaitingFill, Quantity{5}, 0);
  EXPECT_EQ(ob_.DepthAt(OrderSide::kSell, Price{10}), Quantity{5});
  EXPECT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{1}), Quantity{5});
}

TEST_F(OrderBookTest, CancelNonexistentOrder) {
  // Act
  bool result = ob_.Cancel(OrderId{999});

  // Assert
  EXPECT_FALSE(result);
}

TEST_F(OrderBookTest, CancelOnlyOrder) {
  // Arrange
  auto b1 = AddLimitOk(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                       TimeInForce::kGoodTillCancel);

  // Act
  bool result = ob_.Cancel(b1->order_id);

  // Assert
  EXPECT_TRUE(result);
  EXPECT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{1}), Quantity{0});
}

TEST_F(OrderBookTest, CancelFullyTradedOrderFails) {
  // Arrange
  auto b1 = AddLimitOk(UserId{0}, OrderSide::kBuy, Price{10}, Quantity{10},
                       TimeInForce::kGoodTillCancel);
  auto a1 = AddLimitOk(UserId{0}, OrderSide::kSell, Price{10}, Quantity{5},
                       TimeInForce::kGoodTillCancel);
  auto a2 = AddLimitOk(UserId{0}, OrderSide::kSell, Price{10}, Quantity{5},
                       TimeInForce::kGoodTillCancel);

  // Act
  bool result = ob_.Cancel(b1->order_id);

  // Assert
  // A1 takes 5 from B1, immediately filling A1. A2 then takes 5 from B1,
  // resulting in A2 being immediately filled and removing B1 from the book.
  EXPECT_FALSE(result);
  AssertAddResult(b1, OrderStatus::kAwaitingFill, Quantity{10}, 0);
  AssertAddResult(a1, OrderStatus::kImmediateFill, Quantity{0}, 1);
  AssertAddResult(a2, OrderStatus::kImmediateFill, Quantity{0}, 1);
  EXPECT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{10}), Quantity{0});
  EXPECT_EQ(ob_.DepthAt(OrderSide::kSell, Price{10}), Quantity{0});
}
}  // namespace order_book_v1
