#ifndef INCLUDE_ORDERS_H_
#define INCLUDE_ORDERS_H_

#include "types.h"

struct Order {
  OrderId id_;
  UserId creator_id_;
  OrderSide side_;

  Quantity qty_;
  Ticks price_;
};

#endif
