#ifndef INCLUDE_ORDERBOOK_H_
#define INCLUDE_ORDERBOOK_H_

#include <optional>
#include <vector>

#include "order.h"
#include "trade.h"
#include "types.h"

enum class RejectReason {
  kBadPrice = 0,
  kBadQty,
  kOverflow,
  kEmptyBookForMarket,
};

struct AddResult {
  std::optional<RejectReason> error;

  OrderId order_id;
  OrderStatus status;
  std::vector<Trade> immediate_trades;
  Quantity remaining_qty;

};

class OrderBook {
 public:
  // Postconditions: FIFO preserved, empty levels removed, no crossed book
  AddResult add_limit(UserId user_id, OrderSide side, Price price, Quantity qty,
                      TimeInForce tif);
  // Postconditions: FIFO preserved, empty levels removed, no crossed book
  AddResult add_market(UserId user_id, OrderSide side, Quantity qty,
                       TimeInForce tif);

  bool cancel(OrderId order_id);

  std::optional<Price> best_bid();
  std::optional<Price> best_ask();

  Quantity depth_at(OrderSide side, Price price);


};

#endif
