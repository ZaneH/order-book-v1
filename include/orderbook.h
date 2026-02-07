#ifndef INCLUDE_ORDERBOOK_H_
#define INCLUDE_ORDERBOOK_H_

#include <optional>
#include <vector>

#include "trade.h"
#include "types.h"

enum class RejectReason {
  kBadPrice = 0,
  kBadQty,
  kOverflow,
  kEmptyBookForMarket,
};

struct AddResult {
  std::optional<RejectReason> error_;

  OrderId order_id_;
  OrderStatus status_;
  std::vector<Trade> immediate_trades_;
  Quantity remaining_qty_;
};

class OrderBook {
 public:
  // Postconditions: FIFO preserved, empty levels removed, no crossed book
  AddResult add_limit(UserId user_id, OrderSide side, Ticks price, Quantity qty,
                      TimeInForce tif);
  // Postconditions: FIFO preserved, empty levels removed, no crossed book
  AddResult add_market(UserId user_id, OrderSide side, Quantity qty,
                       TimeInForce tif);

  bool cancel(OrderId order_id);

  std::optional<Ticks> best_bid();
  std::optional<Ticks> best_ask();

  Quantity depth_at(OrderSide side, Ticks price);
};

#endif
