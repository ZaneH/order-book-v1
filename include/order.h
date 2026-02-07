#ifndef ORDERS_H
#define ORDERS_H

#include "types.h"

struct Order {
  OrderId id_;
  UserId creator_id_;
  OrderSide side_;

  Quantity qty_;
  Ticks price_;
};

#endif
