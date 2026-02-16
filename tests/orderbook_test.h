#ifndef TESTS_ORDERBOOK_TEST_H_
#define TESTS_ORDERBOOK_TEST_H_

#include <gtest/gtest.h>
#include <orderbook.h>

class OrderBookTest : public testing::Test {
 protected:
  OrderBook ob_;
};

#endif
