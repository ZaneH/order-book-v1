#include "../include/orderbook.h"

#include <cassert>
#include <optional>

Quantity OrderBook::depth_at(OrderSide side, Price price) {
  auto& book_side = side == OrderSide::kBuy ? bids_ : asks_;
  auto it = book_side.find(price);
  if (it == book_side.end()) return Quantity{0};
  return it->second.aggregate_qty;
}

AddResult OrderBook::add_limit(UserId user_id, OrderSide side, Price price,
                               Quantity qty, TimeInForce tif) {
  auto order = Order{
      .id = OrderId{0},
      .creator_id = user_id,
      .side = side,
      .qty = qty,
      .price = price,
      .tif = tif,
  };

#ifndef NDEBUG
  verify();
#endif

  return AddResult{
      .error = std::nullopt,
      .order_id = order.id,
      .status = OrderStatus::kAwaitingFill,
      .immediate_trades = std::vector<Trade>{},
      .remaining_qty = Quantity{0},
  };
}

#ifndef NDEBUG
void OrderBook::verify() const {
  for (auto const& [price, level] : bids_) {
    assert(level.orders.size() != 0);
  }

  for (auto const& [price, level] : asks_) {
    assert(level.orders.size() != 0);
  }
}
#endif
