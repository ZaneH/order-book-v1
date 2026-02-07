#include <gtest/gtest.h>

#include "../include/trade.h"

TEST(Trade, Init) {
  Trade t = Trade();
  EXPECT_EQ(t.qty_, 0);
}
