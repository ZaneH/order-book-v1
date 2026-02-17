#include <optional>

#include "event_log_test.h"

namespace order_book_v1 {
TEST_F(EventLogTest, StringifyEvents) {
  // Arrange
  ArrangeEvents({AddLimitOrderEvent{
                     UserId{6},
                     OrderSide::kBuy,
                     Quantity{2},
                     Price{10},
                     TimeInForce::kGoodTillCancel,
                 },
                 CancelOrderEvent{
                     OrderId{0},
                 },
                 AddMarketOrderEvent{
                     UserId{7},
                     OrderSide::kBuy,
                     Quantity{5},
                 }});

  // Assert
  std::string expected{
      "0 ADDLIMIT 6 BUY 2 10 GTC\n"
      "1 CANCEL 0\n"
      "2 ADDMARKET 7 BUY 5\n"};
  AssertOutput(expected);
  AssertEventSeq(3);
}

TEST_F(EventLogTest, InvalidLimitOrderEvent) {
  // Arrange
  ArrangeEvents({AddLimitOrderEvent{
      UserId{0},
      OrderSide::kBuy,
      Quantity{2},
      std::nullopt,
      std::nullopt,
  }});

  // Assert
  std::string expected{"0 INVALID LIMIT ORDER\n"};
  AssertOutput(expected);
  AssertEventSeq(1);
}
}  // namespace order_book_v1
