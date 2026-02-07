#include "../include/orderbook.h"

#include <gtest/gtest.h>

TEST(OrderBook, AddLimit) {
  OrderBook ob = OrderBook();
  ob.add_limit(UserId{0}, OrderSide::kBuy, Price{1}, Quantity{2},
               TimeInForce::kGoodTillCancel);
}
