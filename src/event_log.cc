#include "../include/event_log.h"

namespace order_book_v1 {
template <typename... Args>
std::ostream& WriteSpaceSep(std::ostream& os, const Args&... xs) {
  bool first = true;
  ((os << (first ? "" : " ") << xs, first = false), ...);
  return os;
}

std::ostream& operator<<(std::ostream& os, const AddLimitOrderEvent& event) {
  if (!event.price.has_value() || !event.tif.has_value())
    return os << "INVALID LIMIT ORDER";
  WriteSpaceSep(os, "ADDLIMIT", event.creator_id, event.side, event.qty,
                event.price.value(), event.tif.value());
  return os;
}

std::ostream& operator<<(std::ostream& os, const AddMarketOrderEvent& event) {
  WriteSpaceSep(os, "ADDMARKET", event.creator_id, event.side, event.qty);
  return os;
}

std::ostream& operator<<(std::ostream& os, const CancelOrderEvent& event) {
  WriteSpaceSep(os, "CANCEL", event.order_id);
  return os;
}

std::ostream& operator<<(std::ostream& os, const LoggedEvent& record) {
  os << record.event_seq << " ";
  std::visit([&os](auto&& args) { os << args; }, record.event);
  return os;
}

EventLog::EventLog(std::ostream* dest) : dest_(dest) {}

void EventLog::AppendEvent(const OrderBookEvent& event) {
  LoggedEvent record{.event_seq = event_seq_++, .event = event};
  *dest_ << record << "\n";
}

uint32_t EventLog::event_seq() { return event_seq_; }
}  // namespace order_book_v1
