#include "orderbook_test.h"

TEST_F(OrderBookTest, AddLimitBuyWithBadQty) {
  auto result = ob_.AddLimit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{0},
                             TimeInForce::kGoodTillCancel);

  ASSERT_EQ(result.error(), RejectReason::kBadQty);
  ASSERT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{1}), Quantity{0});
}

TEST_F(OrderBookTest, AddLimitBuyWithBadPrice) {
  auto result = ob_.AddLimit(UserId{0}, OrderSide::kBuy, Price{0}, Quantity{5},
                             TimeInForce::kGoodTillCancel);

  ASSERT_EQ(result.error(), RejectReason::kBadPrice);
  ASSERT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{0}), Quantity{0});
}

TEST_F(OrderBookTest, AddMarketWithEmptyBook) {
  auto result1 = ob_.AddMarket(UserId{0}, OrderSide::kSell, Quantity{5});
  auto result2 = ob_.AddMarket(UserId{0}, OrderSide::kBuy, Quantity{5});

  ASSERT_EQ(result1.error(), RejectReason::kEmptyBookForMarket);
  ASSERT_EQ(result2.error(), RejectReason::kEmptyBookForMarket);
}
