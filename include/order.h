#ifndef INCLUDE_ORDER_H_
#define INCLUDE_ORDER_H_

#include <optional>

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
}  // namespace order_book_v1

#endif
