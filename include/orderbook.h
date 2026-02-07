#ifndef INCLUDE_ORDERBOOK_H_
#define INCLUDE_ORDERBOOK_H_

#include <list>
#include <map>
#include <optional>
#include <unordered_map>
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

struct Level {
  Quantity aggregate_qty{};
  std::list<Order> orders;
};

struct Handle {
  OrderSide side;
  using SideMap = std::map<Price, Level>;
  SideMap::iterator level_it;
  std::list<Order>::iterator order_it;
};

class OrderBook {
 public:
  // Postconditions: FIFO preserved, empty levels removed, no crossed book
  AddResult add_limit(UserId user_id, OrderSide side, Price price, Quantity qty,
                      TimeInForce tif);
  // Postconditions: FIFO preserved, empty levels removed, no crossed book
  AddResult add_market(UserId user_id, OrderSide side, Quantity qty,
                       TimeInForce tif);

  // Cancelling an order is O(1) because the location of every order is stored
  // in the order_id_index_ class data member as a Handle. Cancelling an order
  // will not require shifting any elements because std::list is being used to
  // store orders.
  bool cancel(OrderId order_id);

  std::optional<Price> best_bid();
  std::optional<Price> best_ask();

  Quantity depth_at(OrderSide side, Price price);

 private:
  std::map<Price, Level> bids_;
  std::map<Price, Level> asks_;

  std::unordered_map<OrderId, Handle, StrongIdHash<OrderIdTag>> order_id_index_;

#ifndef NDEBUG
  // Only provided in debug builds. For verifying invariants.
  void verify() const;
#endif
};

#endif
