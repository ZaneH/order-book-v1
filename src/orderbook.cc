#include "../include/orderbook.h"

#include <cassert>
#include <expected/expected.hpp>
#include <iostream>
#include <iterator>
#include <map>
#include <optional>
#include <ostream>
#include <unordered_map>
#include <utility>

Quantity OrderBook::DepthAt(OrderSide side, Price price) {
  auto& book_side = side == OrderSide::kBuy ? bids_ : asks_;
  auto it = book_side.find(price);
  if (it == book_side.end()) return Quantity{0};
  return it->second.aggregate_qty;
}

std::optional<Price> OrderBook::BestAsk() {
  if (asks_.empty()) return std::nullopt;
  return asks_.begin()->first;
}

std::optional<Price> OrderBook::BestBid() {
  if (bids_.empty()) return std::nullopt;
  return bids_.rbegin()->first;
}

MatchResult OrderBook::Match(OrderSide side, Price value, const Order& order) {
  std::vector<Trade> trades{};
  std::optional<Order> unfilled{};

  BookSide* other_side = (side == OrderSide::kBuy) ? &asks_ : &bids_;

  Quantity qty_unfilled = order.qty;
  auto& level = other_side->at(value);
  while (qty_unfilled > Quantity{0}) {
    if (level.orders.empty()) {
      unfilled = order;
      unfilled->qty = qty_unfilled;
      break;
    }

    Order& first_in_level = level.orders.front();
    Quantity fill_amount =
        first_in_level.qty < qty_unfilled ? first_in_level.qty : qty_unfilled;

    first_in_level.qty -= fill_amount;
    level.aggregate_qty -= fill_amount;
    qty_unfilled -= fill_amount;
    if (first_in_level.qty == Quantity{0}) {
      auto handle_it = order_id_index_.at(first_in_level.id);
      auto order_it = handle_it.order_it;
      level.orders.erase(order_it);
      order_id_index_.erase(first_in_level.id);
    }

    // TODO: Might be able to be merged with earlier check
    if (level.orders.empty()) {
      other_side->erase(value);
    }

    trades.emplace_back(Trade{
        .maker_id = first_in_level.creator_id,
        .taker_id = order.creator_id,
        .match_id = MatchId{0},  // TODO: Create Match nonce
        .qty = fill_amount,
        .price = first_in_level.price,
    });
  }

  return MatchResult{.trades = trades,
                     .unfilled = unfilled,
                     .filled_all = qty_unfilled == Quantity{0}};
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

  auto* book_side = (side == OrderSide::kBuy) ? &bids_ : &asks_;

  auto best_ask = BestAsk();
  auto best_bid = BestBid();

  MatchResult cross_match{};
  if (side == OrderSide::kBuy && best_ask.has_value() &&
      order.price >= best_ask.value()) {
    std::cerr << "Crossing case for incoming Buy" << std::endl;
    cross_match = Match(side, best_ask.value(), order);
  } else if (side == OrderSide::kSell && best_bid.has_value() &&
             order.price <= best_bid.value()) {
    std::cerr << "Crossing case for incoming Sell" << std::endl;
    cross_match = Match(side, best_bid.value(), order);
  }

  if (cross_match.unfilled.has_value()) {
    return AddResultPayload{
        .order_id = order.id,
        .status = OrderStatus::kPartialFill,
        .immediate_trades = cross_match.trades,
        .remaining_qty = cross_match.unfilled->qty,
    };
  } else if (cross_match.filled_all) {
    return AddResultPayload{
        .order_id = order.id,
        .status = OrderStatus::kImmediateFill,
        .immediate_trades = cross_match.trades,
        .remaining_qty = Quantity{0},
    };
  }

  auto [level_it, inserted] = book_side->try_emplace(
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
