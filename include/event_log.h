#ifndef INCLUDE_EVENT_LOG_H_
#define INCLUDE_EVENT_LOG_H_

#include <cstdint>
#include <optional>
#include <ostream>
#include <variant>

#include "types.h"

namespace order_book_v1 {
struct AddLimitOrderEvent {
  UserId creator_id;
  OrderSide side;
  Quantity qty;
  std::optional<Price> price;
  std::optional<TimeInForce> tif;
};

struct AddMarketOrderEvent {
  UserId creator_id;
  OrderSide side;
  Quantity qty;
};

struct CancelOrderEvent {
  OrderId order_id;
};

using OrderBookEvent =
    std::variant<AddLimitOrderEvent, AddMarketOrderEvent, CancelOrderEvent>;

struct LoggedEvent {
  uint32_t event_seq;
  OrderBookEvent event;
};

class EventLog {
 public:
  EventLog(std::ostream* dst);

  std::ostream* dst_stream();
  void AppendEvent(const OrderBookEvent& event);
  uint32_t event_seq();

 private:
  std::ostream* dst_;
  uint32_t event_seq_ = 0;
};
}  // namespace order_book_v1

#endif
