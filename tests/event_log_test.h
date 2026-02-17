#ifndef TESTS_EVENT_LOG_TEST_H_
#define TESTS_EVENT_LOG_TEST_H_

#include <gtest/gtest.h>

#include <cstdint>
#include <initializer_list>
#include <sstream>
#include <string_view>

#include "event_log.h"

namespace order_book_v1 {
class EventLogTest : public testing::Test {
 protected:
  std::ostringstream buf_;
  EventLog log_{&buf_};

  void ArrangeEvents(const std::initializer_list<OrderBookEvent>& events);
  void AssertOutput(std::string_view expected);
  void AssertEventSeq(uint32_t expected);
};
}  // namespace order_book_v1

#endif
