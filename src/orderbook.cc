#include "../include/orderbook.h"

#include <cassert>
#include <expected/expected.hpp>
#include <iterator>
#include <map>
#include <optional>
#include <unordered_map>
#include <utility>

namespace order_book_v1 {
void OrderBook::EmitLimitOrderEvent(const Order& order) {
  log_.AppendEvent(AddLimitOrderEvent{
      .creator_id = order.creator_id,
      .side = order.side,
      .qty = order.qty,
      .price = order.price,
      .tif = order.tif,
  });
}

void OrderBook::EmitMarketOrderEvent(const Order& order) {
  log_.AppendEvent(AddMarketOrderEvent{
      .creator_id = order.creator_id,
      .side = order.side,
      .qty = order.qty,
  });
}

void OrderBook::EmitCancelEvent(const OrderId& id) {
  log_.AppendEvent(CancelOrderEvent{.order_id = id});
}

Quantity OrderBook::DepthAt(OrderSide side, Price price) const {
  auto& book_side = side == OrderSide::kBuy ? bids_ : asks_;
  auto it = book_side.find(price);
  if (it == book_side.end()) return Quantity{0};
  return it->second.aggregate_qty;
}

std::optional<Price> OrderBook::BestBid() const {
  if (bids_.empty()) return std::nullopt;
  return bids_.rbegin()->first;
}

std::optional<Price> OrderBook::BestAsk() const {
  if (asks_.empty()) return std::nullopt;
  return asks_.begin()->first;
}

void OrderBook::AddOrderToBook(OrderSide side, BookSide* book_side, Price value,
                               const Order& order) {
  auto [level_it, inserted] = book_side->try_emplace(
      value, Level{.aggregate_qty = Quantity{0}, .orders = {}});

  Level& level = level_it->second;

  level.orders.emplace_back(order);
  auto order_it = std::prev(level.orders.end());

  level.aggregate_qty += order.qty;

  order_id_index_.emplace(
      order.id,
      Handle{.side = side, .level_it = level_it, .order_it = order_it});
}

// Fills against the front order in level, updates book and trade log
void OrderBook::Reduce(Level& level, Quantity& unfilled_qty, const Order& order,
                       std::vector<Trade>& trades) {
  Order& first_in_level = level.orders.front();
  Quantity fill_amount =
      first_in_level.qty < unfilled_qty ? first_in_level.qty : unfilled_qty;

  first_in_level.qty -= fill_amount;
  level.aggregate_qty -= fill_amount;
  unfilled_qty -= fill_amount;

  trades.emplace_back(Trade{
      .maker_id = first_in_level.creator_id,
      .taker_id = order.creator_id,
      .match_id = MatchId{match_id_++},
      .order_id = order.id,
      .qty = fill_amount,
      .price = first_in_level.price.value(),
  });

  if (first_in_level.qty == Quantity{0}) {
    Handle& handle = order_id_index_.at(first_in_level.id);
    auto order_it = handle.order_it;
    order_id_index_.erase(first_in_level.id);
    level.orders.erase(order_it);
  }
}

MatchResult OrderBook::Match(OrderSide side, Price best_price,
                             const Order& order, bool is_market) {
  std::vector<Trade> trades{};
  std::optional<Order> unfilled{};

  BookSide* other_side = (side == OrderSide::kBuy) ? &asks_ : &bids_;

  Quantity unfilled_qty = order.qty;
  Level* level = &other_side->at(best_price);

  while (unfilled_qty > Quantity{0}) {
    if (!other_side->contains(best_price)) {
      auto next_best = (side == OrderSide::kBuy) ? BestAsk() : BestBid();
      bool will_accept = (side == OrderSide::kBuy) ? next_best <= order.price
                                                   : next_best >= order.price;
      if ((is_market || will_accept) && next_best.has_value()) {
        level = &other_side->at(next_best.value());

        Reduce(*level, unfilled_qty, order, trades);
        if (level->orders.empty()) {
          other_side->erase(next_best.value());
        }
        continue;
      } else {
        unfilled = order;
        unfilled->qty = unfilled_qty;
        break;
      }
    }

    Reduce(*level, unfilled_qty, order, trades);
    if (level->orders.empty()) {
      other_side->erase(best_price);
    }
  }

  return MatchResult{.trades = trades,
                     .unfilled = unfilled,
                     .filled_all = unfilled_qty == Quantity{0}};
}

AddResult OrderBook::AddMarket(UserId user_id, OrderSide side, Quantity qty) {
  if (qty == Quantity{0}) {
    return tl::unexpected<RejectReason>(RejectReason::kBadQty);
  }

  auto const& order = Order{.id = OrderId{order_id_++},
                            .creator_id = user_id,
                            .side = side,
                            .qty = qty,
                            .price = std::nullopt,
                            .tif = std::nullopt};
  auto best_value = (side == OrderSide::kBuy) ? BestAsk() : BestBid();
  if (!best_value.has_value()) {
    return tl::unexpected<RejectReason>(RejectReason::kEmptyBookForMarket);
  }

  MatchResult cross_match{};
  if (side == OrderSide::kBuy) {
    cross_match = Match(side, best_value.value(), order, true);
  } else if (side == OrderSide::kSell) {
    cross_match = Match(side, best_value.value(), order, true);
  }

  if (cross_match.unfilled.has_value()) {
    EmitMarketOrderEvent(order);
    return AddResultPayload{
        .order_id = order.id,
        .status = OrderStatus::kPartialFill,
        .immediate_trades = cross_match.trades,
        .remaining_qty = cross_match.unfilled->qty,
    };
  } else if (cross_match.filled_all) {
    EmitMarketOrderEvent(order);
    return AddResultPayload{
        .order_id = order.id,
        .status = OrderStatus::kImmediateFill,
        .immediate_trades = cross_match.trades,
        .remaining_qty = Quantity{0},
    };
  }

#ifndef NDEBUG
  Verify();
#endif

  return tl::unexpected<RejectReason>(RejectReason::kEmptyBookForMarket);
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
      .id = OrderId{order_id_++},
      .creator_id = user_id,
      .side = side,
      .qty = qty,
      .price = price,
      .tif = tif,
  };

  auto* book_side = (side == OrderSide::kBuy) ? &bids_ : &asks_;
  auto best_value = (side == OrderSide::kBuy) ? BestAsk() : BestBid();

  MatchResult cross_match{};
  if (side == OrderSide::kBuy && best_value.has_value() &&
      order.price >= best_value.value()) {
    cross_match = Match(side, best_value.value(), order, false);
  } else if (side == OrderSide::kSell && best_value.has_value() &&
             order.price <= best_value.value()) {
    cross_match = Match(side, best_value.value(), order, false);
  }

  bool discard_remainder = tif == TimeInForce::kImmediateOrCancel;
  if (cross_match.unfilled.has_value()) {
    if (!discard_remainder) {
      AddOrderToBook(side, book_side, price, cross_match.unfilled.value());
    }
    EmitLimitOrderEvent(order);
    return AddResultPayload{
        .order_id = order.id,
        .status = OrderStatus::kPartialFill,
        .immediate_trades = cross_match.trades,
        .remaining_qty =
            discard_remainder ? Quantity{0} : cross_match.unfilled->qty,
    };
  } else if (cross_match.filled_all) {
    EmitLimitOrderEvent(order);
    return AddResultPayload{
        .order_id = order.id,
        .status = OrderStatus::kImmediateFill,
        .immediate_trades = cross_match.trades,
        .remaining_qty = Quantity{0},
    };
  }

  if (!discard_remainder) {
    AddOrderToBook(side, book_side, price, order);
  }

#ifndef NDEBUG
  Verify();
#endif

  EmitLimitOrderEvent(order);
  return AddResultPayload{
      .order_id = order.id,
      .status = OrderStatus::kAwaitingFill,
      .immediate_trades = std::vector<Trade>{},
      .remaining_qty = discard_remainder ? Quantity{0} : Quantity{order.qty},
  };
}

bool OrderBook::Cancel(OrderId id) {
  auto handle_it = order_id_index_.find(id);
  if (handle_it == order_id_index_.end()) {
    return false;
  }
  Handle& handle = handle_it->second;
  auto& level_it = handle.level_it;
  Level& level = level_it->second;

  level.aggregate_qty -= handle.order_it->qty;
  level.orders.erase(handle.order_it);
  if (level.orders.empty()) {
    BookSide* book_side = (handle.side == OrderSide::kBuy) ? &bids_ : &asks_;
    book_side->erase(level_it);
  }

  order_id_index_.erase(handle_it);
#ifndef NDEBUG
  Verify();
#endif

  EmitCancelEvent(id);
  return true;
}

#ifndef NDEBUG
void VerifyAggregateQtyPerLevel(BookSide book_side) {
  for (auto const& [price, level] : book_side) {
    Quantity level_qty_sum{};

    for (auto const& order : level.orders) {
      level_qty_sum += order.qty;
    }

    assert(level.aggregate_qty == level_qty_sum);
    assert(!level.orders.empty());
  }
}

void VerifyNoEmptyLevelsOrEmptyOrders(BookSide book_side) {
  for (auto const& [price, level] : book_side) {
    assert(!level.orders.empty());

    for (auto const order : level.orders) {
      assert(order.qty != Quantity{0});
    }
  }
}

void OrderBook::Verify() const {
  VerifyAggregateQtyPerLevel(bids_);
  VerifyAggregateQtyPerLevel(asks_);

  VerifyNoEmptyLevelsOrEmptyOrders(bids_);
  VerifyNoEmptyLevelsOrEmptyOrders(asks_);
}
#endif
}  // namespace order_book_v1
