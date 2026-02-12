#include "../include/orderbook.h"

#include <cassert>
#include <expected/expected.hpp>
#include <map>
#include <unordered_map>
#include <utility>

Quantity OrderBook::DepthAt(OrderSide side, Price price) {
  auto& book_side = side == OrderSide::kBuy ? bids_ : asks_;
  auto it = book_side.find(price);
  if (it == book_side.end()) return Quantity{0};
  return it->second.aggregate_qty;
}

AddResult OrderBook::AddLimit(UserId user_id, OrderSide side, Price price,
                              Quantity qty, TimeInForce tif) {
  if (qty == Quantity{0}) {
    return tl::unexpected<RejectReason>(RejectReason::kBadQty);
  }
  if (price == Price{0}) {
    return tl::unexpected<RejectReason>(RejectReason::kBadPrice);
  }

  auto const& order = Order{
      .id = OrderId{order_nonce_},
      .creator_id = user_id,
      .side = side,
      .qty = qty,
      .price = price,
      .tif = tif,
  };

  auto& book_side = (side == OrderSide::kBuy) ? bids_ : asks_;

  // if (side == OrderSide::kBuy && order.price >= BestAsk()) {
  //   // Cross, immediate match
  // } else if (side == OrderSide::kSell && order.price <= BestBid()) {
  //   // Cross, immediate match
  // }

  auto [level_it, inserted] = book_side.try_emplace(
      price, Level{.aggregate_qty = Quantity{0}, .orders = {}});

  Level& level = level_it->second;

  level.orders.emplace_back(order);
  auto order_it = std::prev(level.orders.end());

  level.aggregate_qty += order.qty;

  order_id_index_.emplace(
      order.id,
      Handle{.side = side, .level_it = level_it, .order_it = order_it});

  order_nonce_++;

#ifndef NDEBUG
  Verify();
#endif

  return AddResultPayload{
      .order_id = order.id,
      .status = OrderStatus::kAwaitingFill,
      .immediate_trades = std::vector<Trade>{},
      .remaining_qty = Quantity{0},
  };
}

bool OrderBook::Cancel(OrderId id) {
  auto handle_it = order_id_index_.find(id);
  if (handle_it == order_id_index_.end()) {
    return false;
  }
  Handle& handle = handle_it->second;
  Level& level = handle.level_it->second;

  level.aggregate_qty -= handle.order_it->qty;
  level.orders.erase(handle.order_it);
  order_id_index_.erase(handle_it);
  return true;
}

#ifndef NDEBUG
void verify_side(std::map<Price, Level> book_side) {
  for (auto const& [price, level] : book_side) {
    Quantity level_qty_sum{};

    for (auto const& order : level.orders) {
      level_qty_sum += order.qty;
    }

    assert(level.aggregate_qty == level_qty_sum);
    assert(level.orders.size() != 0);
  }
}

void OrderBook::Verify() const {
  verify_side(bids_);
  verify_side(asks_);
}
#endif
