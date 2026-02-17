#include "../include/event_log.h"

namespace order_book_v1 {
template <typename... Args>
std::ostream& WriteSpaceSep(std::ostream& os, const Args&... xs) {
  bool first = true;
  ((os << (first ? "" : " ") << xs, first = false), ...);
  return os;
}

std::ostream& operator<<(std::ostream& os, const AddLimitOrderEvent& event) {
  WriteSpaceSep(os, "ADDLIMIT", event.event_seq, event.creator_id, event.side,
                // TODO: Guard against nullopt price/tif values
                event.qty, event.price.value(), event.tif.value());
  return os;
}

std::ostream& operator<<(std::ostream& os, const AddMarketOrderEvent& event) {
  WriteSpaceSep(os, "ADDMARKET", event.event_seq, event.creator_id, event.side,
                event.qty);
  return os;
}

std::ostream& operator<<(std::ostream& os, const CancelOrderEvent& event) {
  WriteSpaceSep(os, "CANCEL", event.event_seq, event.order_id);
  return os;
}

void EventLog::AppendEvent(const OrderBookEvent& event, std::ostream& os) {
  std::visit([&os](auto&& args) { os << args << std::endl; }, event);
}
}  // namespace order_book_v1
