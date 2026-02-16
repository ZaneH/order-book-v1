#include "orderbook_test.h"

namespace order_book_v1 {
TEST_F(OrderBookTest, AddLimitBuyWithBadQty) {
  // Act
  auto result = AddLimitError(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{0},
                              TimeInForce::kGoodTillCancel);

  // Assert
  EXPECT_EQ(result.error(), RejectReason::kBadQty);
  EXPECT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{1}), Quantity{0});
}

TEST_F(OrderBookTest, AddLimitBuyWithBadPrice) {
  // Act
  auto result = AddLimitError(UserId{0}, OrderSide::kBuy, Price{0}, Quantity{5},
                              TimeInForce::kGoodTillCancel);

  // Assert
  EXPECT_EQ(result.error(), RejectReason::kBadPrice);
  EXPECT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{0}), Quantity{0});
}

TEST_F(OrderBookTest, AddMarketWithEmptyBook) {
  // Act
  auto result1 = AddMarketError(UserId{0}, OrderSide::kSell, Quantity{5});
  auto result2 = AddMarketError(UserId{0}, OrderSide::kBuy, Quantity{5});

  // Assert
  EXPECT_EQ(result1.error(), RejectReason::kEmptyBookForMarket);
  EXPECT_EQ(result2.error(), RejectReason::kEmptyBookForMarket);
}
}  // namespace order_book_v1
