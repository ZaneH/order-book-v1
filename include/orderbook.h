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
  AddResult add_limit(UserId user_id, OrderSide side, Ticks price, Quantity qty,
                      TimeInForce tif);
  AddResult add_market(UserId user_id, OrderSide side, Quantity qty,
                       TimeInForce tif);
};
