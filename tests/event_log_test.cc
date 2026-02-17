#include "event_log.h"

#include <gtest/gtest.h>

#include <sstream>

#include "event_log_test.h"

namespace order_book_v1 {
void EventLogTest::ArrangeEvents(
    const std::initializer_list<OrderBookEvent>& events) {
  for (const auto& e : events) {
    log_.AppendEvent(e, buf_);
  }
}

void EventLogTest::AssertOutput(std::string_view expected) {
  EXPECT_EQ(buf_.str(), expected);
}

void EventLogTest::AssertEventSeq(uint32_t expected) {
  EXPECT_EQ(log_.event_seq(), expected);
}
}  // namespace order_book_v1
