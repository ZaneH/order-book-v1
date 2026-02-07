#ifndef INCLUDE_ORDERS_H_
#define INCLUDE_ORDERS_H_

#include "types.h"

struct Order {
  OrderId id;
  UserId creator_id;
  OrderSide side;

  Quantity qty;
  Price price;
};

#endif
