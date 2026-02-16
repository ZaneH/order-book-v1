#include "orderbook_test.h"

namespace order_book_v1 {
TEST_F(OrderBookTest, AddLimitCrossingImmediateFill) {
  // Arrange
  ArrangeBidLevels({{Price{10}, Quantity{10}}});

  // Act
  auto result = AddLimitOk(UserId{1}, OrderSide::kSell, Price{10}, Quantity{5},
                           TimeInForce::kGoodTillCancel);

  // Assert
  // A1 takes 5 from B1 leaving B1 with 5 unfilled
  AssertAddResult(result, OrderStatus::kImmediateFill, Quantity{0}, 1);
  EXPECT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{10}), Quantity{5});
  EXPECT_EQ(ob_.DepthAt(OrderSide::kSell, Price{10}), Quantity{0});
}

TEST_F(OrderBookTest, AddLimitCrossingPartialFill) {
  // Arrange
  ArrangeBidLevels({{Price{10}, Quantity{10}}, {Price{5}, Quantity{2}}});

  // Act
  auto result = ob_.AddLimit(UserId{0}, OrderSide::kSell, Price{10},
                             Quantity{20}, TimeInForce::kGoodTillCancel);

  /*
   * Incoming: A1 wants to sell 20 @ 10
   *
   * [bids]
   * 10: B1(10)
   * 5:  B2(2)
   */

  // Assert
  // A1 takes 10 from B1, resulting in B1 being removed from the book and
  // leaving A1 with 10 unfilled. B2 is untouched, resting in the book.
  AssertAddResult(result, OrderStatus::kPartialFill, Quantity{10}, 1);
  EXPECT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{10}), Quantity{0});
  EXPECT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{5}), Quantity{2});
  EXPECT_EQ(ob_.DepthAt(OrderSide::kSell, Price{10}), Quantity{10});
}

TEST_F(OrderBookTest, AddLimitCrossingMultipleLevels) {
  // Arrange
  ArrangeAskLevels({{Price{15}, Quantity{10}}, {Price{10}, Quantity{5}}});

  // Act
  auto result = ob_.AddLimit(UserId{0}, OrderSide::kBuy, Price{20},
                             Quantity{10}, TimeInForce::kGoodTillCancel);

  /*
   * Incoming: B1 wants to buy 10 @ 20
   *
   * [asks]
   * 10: A2(5)
   * 15: A1(10)
   */

  // Assert
  // B1 takes 5 from A2, resulting in A2 being removed from the book.
  // B1 takes 5 from A1, filling B1. A1 is left with 5 unfilled.
  AssertAddResult(result, OrderStatus::kImmediateFill, Quantity{0}, 2);
  EXPECT_EQ(ob_.DepthAt(OrderSide::kSell, Price{15}), Quantity{5});
  EXPECT_EQ(ob_.DepthAt(OrderSide::kSell, Price{10}), Quantity{0});
  EXPECT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{20}), Quantity{0});
}

TEST_F(OrderBookTest, AddMarketSingleSellImmediateFill) {
  // Arrange
  ArrangeBidLevels({{Price{10}, Quantity{10}}});

  // Act
  auto result = ob_.AddMarket(UserId{0}, OrderSide::kSell, Quantity{5});

  // Assert
  EXPECT_EQ(result->status, OrderStatus::kImmediateFill);
  EXPECT_EQ(result->remaining_qty, Quantity{0});
  EXPECT_EQ(result->immediate_trades.size(), 1);
  EXPECT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{10}), Quantity{5});
  EXPECT_EQ(ob_.DepthAt(OrderSide::kSell, Price{10}), Quantity{0});
}

TEST_F(OrderBookTest, AddMarketSingleSellDiscardUnfilled) {
  // Arrange
  ArrangeBidLevels({{Price{10}, Quantity{10}}});

  // Act
  auto result = ob_.AddMarket(UserId{0}, OrderSide::kSell, Quantity{50});

  // Assert
  AssertAddResult(result, OrderStatus::kPartialFill, Quantity{40}, 1);
  EXPECT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{10}), Quantity{0});
  EXPECT_EQ(ob_.DepthAt(OrderSide::kSell, Price{10}), Quantity{0});
}

TEST_F(OrderBookTest, AddMarketSingleSellMultipleLevels) {
  // Arrange
  ArrangeBidLevels({{Price{10}, Quantity{10}}, {Price{8}, Quantity{10}}});

  // Act
  auto result = ob_.AddMarket(UserId{0}, OrderSide::kSell, Quantity{50});

  /*
   * Incoming: A1 wants to sell 50 @ Market
   *
   * [bids]
   * 10: B1(10)
   * 8:  B2(10)
   */

  // Assert
  // A1 takes 10 from B1, resulting in B1 being removed from the book.
  // A1 takes 10 from B2, resulting in B2 being removed from the book.
  // A1 is left with 30 unfilled and the remaining is discarded.
  AssertAddResult(result, OrderStatus::kPartialFill, Quantity{30}, 2);
  EXPECT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{10}), Quantity{0});
  EXPECT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{8}), Quantity{0});
  EXPECT_EQ(ob_.DepthAt(OrderSide::kSell, Price{10}), Quantity{0});
  EXPECT_EQ(ob_.DepthAt(OrderSide::kSell, Price{8}), Quantity{0});
}

TEST_F(OrderBookTest, AddMarketSingleBuyMultipleLevels) {
  // Arrange
  ArrangeAskLevels({{Price{10}, Quantity{10}}, {Price{8}, Quantity{10}}});

  // Act
  auto result = ob_.AddMarket(UserId{0}, OrderSide::kBuy, Quantity{50});

  // Assert
  AssertAddResult(result, OrderStatus::kPartialFill, Quantity{30}, 2);
  EXPECT_EQ(ob_.DepthAt(OrderSide::kSell, Price{10}), Quantity{0});
  EXPECT_EQ(ob_.DepthAt(OrderSide::kSell, Price{8}), Quantity{0});
  EXPECT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{10}), Quantity{0});
  EXPECT_EQ(ob_.DepthAt(OrderSide::kBuy, Price{8}), Quantity{0});
}
}  // namespace order_book_v1
