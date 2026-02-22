#include <gtest/gtest.h>

#include "hash.h"
#include "orderbook.h"
#include "types.h"

namespace order_book_v1 {
TEST(OrderBook, DifferentOrderBookHashUserIdChange) {
  // Arrange
  auto ob1 = OrderBook();
  auto result1 = ob1.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                              TimeInForce::kGoodTillCancel);
  auto result2 = ob1.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                              TimeInForce::kGoodTillCancel);

  auto ob2 = OrderBook();
  auto result3 = ob2.AddLimit(UserId{1}, OrderSide::kBuy, Price{1}, Quantity{5},
                              TimeInForce::kGoodTillCancel);
  auto result4 = ob2.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                              TimeInForce::kGoodTillCancel);

  // Act
  FixedWidth h1 = ob1.ToHash();
  FixedWidth h2 = ob2.ToHash();

  // Assert
  EXPECT_NE(h1, h2);
}

TEST(OrderBook, DifferentOrderBookHashNewOrder) {
  // Arrange
  auto ob1 = OrderBook();
  auto result1 = ob1.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                              TimeInForce::kGoodTillCancel);
  auto result2 = ob1.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                              TimeInForce::kGoodTillCancel);

  auto ob2 = ob1;
  auto result3 = ob1.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                              TimeInForce::kGoodTillCancel);

  // Act
  FixedWidth h1 = ob1.ToHash();
  FixedWidth h2 = ob2.ToHash();

  // Assert
  EXPECT_NE(h1, h2);
}

TEST(OrderBook, EqualOrderBookHashIdenticalState) {
  // Arrange
  auto ob1 = OrderBook();
  auto result1 = ob1.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                              TimeInForce::kGoodTillCancel);
  auto result2 = ob1.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                              TimeInForce::kGoodTillCancel);

  auto ob2 = OrderBook();
  auto result3 = ob2.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                              TimeInForce::kGoodTillCancel);
  auto result4 = ob2.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{5},
                              TimeInForce::kGoodTillCancel);

  // Act
  FixedWidth h1 = ob1.ToHash();
  FixedWidth h2 = ob2.ToHash();

  // Assert
  EXPECT_EQ(h1, h2);
}
}  // namespace order_book_v1
