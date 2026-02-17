#ifndef INCLUDE_EVENT_LOG_H_
#define INCLUDE_EVENT_LOG_H_

#include <optional>
#include <variant>

#include "types.h"

namespace order_book_v1 {
enum class EventLogDestination {
  kStdout = 0,
};

struct BaseEvent {
  uint32_t event_seq;
};

struct AddLimitOrderEvent : BaseEvent {
  UserId creator_id;
  OrderSide side;

  Quantity qty;
  std::optional<Price> price;

  std::optional<TimeInForce> tif;
};

struct AddMarketOrderEvent : BaseEvent {
  UserId creator_id;
  OrderSide side;

  Quantity qty;
};

struct CancelOrderEvent : BaseEvent {
  OrderId order_id;
};

using OrderBookEvent =
    std::variant<AddLimitOrderEvent, AddMarketOrderEvent, CancelOrderEvent>;

class EventLog {
 public:
  void AppendEvent(const OrderBookEvent& event, std::ostream& os);

 private:
  EventLogDestination dest_;  // TODO: Make use of dest_
};
}  // namespace order_book_v1

#endif
