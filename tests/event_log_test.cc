#include "event_log.h"

#include <gtest/gtest.h>

namespace order_book_v1 {
TEST(EventLog, ProofOfConcept) {
  EventLog log;

  AddLimitOrderEvent e1 = {BaseEvent{.event_seq = 0},
                           UserId{0},
                           OrderSide::kBuy,
                           Quantity{2},
                           Price{10},
                           TimeInForce::kGoodTillCancel};
  CancelOrderEvent e2 = {BaseEvent{.event_seq = 1}, OrderId{0}};

  log.AppendEvent(e1, std::cout);
  log.AppendEvent(e2, std::cout);
}
}  // namespace order_book_v1
