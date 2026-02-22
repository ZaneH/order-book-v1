#ifndef INCLUDE_ORDER_H_
#define INCLUDE_ORDER_H_

#include <optional>

#include "hash.h"
#include "types.h"

namespace order_book_v1 {
struct Order {
  OrderId id;
  UserId creator_id;
  OrderSide side;

  Quantity qty;
  std::optional<Price> price;

  std::optional<TimeInForce> tif;
};

inline void HashOrder(FixedWidth& seed, Order const& order) {
  HashCombine(seed, order.id.v);
  HashCombine(seed, order.creator_id.v);
  HashCombine(seed, order.side);
  HashCombine(seed, order.qty.v);
  if (order.price.has_value()) {
    HashCombine(seed, order.price.value().v);
  }
  if (order.tif.has_value()) {
    HashCombine(seed, order.tif.value());
  }
}
}  // namespace order_book_v1

#endif
