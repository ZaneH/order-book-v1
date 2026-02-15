#ifndef INCLUDE_ORDERS_H_
#define INCLUDE_ORDERS_H_

#include <optional>

#include "types.h"

struct Order {
  OrderId id;
  UserId creator_id;
  OrderSide side;

  Quantity qty;
  std::optional<Price> price;

  std::optional<TimeInForce> tif;
};

#endif
