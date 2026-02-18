#ifndef INCLUDE_ORDERBOOK_H_
#define INCLUDE_ORDERBOOK_H_

#include <cstdint>
#include <expected/expected.hpp>
#include <iostream>
#include <list>
#include <map>
#include <optional>
#include <ostream>
#include <unordered_map>
#include <vector>

#include "event_log.h"
#include "order.h"
#include "trade.h"
#include "types.h"

namespace order_book_v1 {
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

enum class RejectReason {
  kBadPrice = 0,
  kBadQty,
  kOverflow,  // NOTE: Currently unused
  kEmptyBookForMarket,
};

struct AddResultPayload {
  OrderId order_id;
  OrderStatus status;
  std::vector<Trade> immediate_trades;
  Quantity remaining_qty;
};

using AddResult = tl::expected<AddResultPayload, RejectReason>;
using BookSide = std::map<Price, Level>;

struct MatchResult {
  std::vector<Trade> trades;
  std::optional<Order> unfilled;
  bool filled_all;
};

class OrderBook {
 public:
  OrderBook(std::ostream* log_dest = &std::cout);

  // Postconditions: FIFO preserved, empty levels removed, no crossed book
  AddResult AddLimit(UserId user_id, OrderSide side, Price price, Quantity qty,
                     TimeInForce tif);
  // Postconditions: FIFO preserved, empty levels removed, no crossed book
  AddResult AddMarket(UserId user_id, OrderSide side, Quantity qty);

  // Cancelling an order is O(1) because the location of every order is stored
  // in the order_id_index_ class data member as a Handle. Cancelling an order
  // will not require shifting any elements because std::list is being used to
  // store orders.
  bool Cancel(OrderId order_id);

  std::optional<Price> BestBid() const;
  std::optional<Price> BestAsk() const;

  Quantity DepthAt(OrderSide side, Price price) const;

  friend std::ostream& operator<<(std::ostream& os, const OrderBook& book) {
    os << "Book:";
    if (book.bids_.empty() && book.asks_.empty()) {
      os << "\t(empty)\n";
      return os;
    }

    if (!book.bids_.empty()) {
      os << "\n[bids]\n";
      for (const auto& [price, level] : book.bids_) {
        os << price.v << ":\t";
        for (const auto& order : level.orders) {
          os << "B" << order.id.v << "(" << order.qty.v << "), ";
        }
        os << "\n";
      }
    } else {
      os << "\n";
    }

    if (!book.asks_.empty()) {
      os << "[asks]\n";
      for (const auto& [price, level] : book.asks_) {
        os << price.v << ":\t";
        for (const auto& order : level.orders) {
          os << "A" << order.id.v << "(" << order.qty.v << "), ";
        }
        os << "\n";
      }
    }

    return os;
  };

 private:
  std::map<Price, Level> bids_;
  std::map<Price, Level> asks_;

  uint32_t order_id_ = 0;
  uint32_t match_id_ = 0;

  std::unordered_map<OrderId, Handle, StrongIdHash<OrderIdTag>> order_id_index_;

  MatchResult Match(OrderSide side, Price best_value, const Order& order,
                    bool is_market);
  void Reduce(Level& level, Quantity& unfilled_qty, const Order& order,
              std::vector<Trade>& trades);
  void AddOrderToBook(OrderSide side, BookSide* book_side, Price value,
                      const Order& order);
  void EmitLimitOrderEvent(const Order& order);
  void EmitMarketOrderEvent(const Order& order);
  void EmitCancelEvent(OrderId order);

  EventLog log_;

#ifndef NDEBUG
  // Only provided in debug builds. Used to verify invariants.
  void Verify() const;
#endif
};
}  // namespace order_book_v1

#endif
