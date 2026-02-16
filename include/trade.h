#ifndef INCLUDE_TRADE_H_
#define INCLUDE_TRADE_H_

#include "types.h"

namespace order_book_v1 {
struct Trade {
  UserId maker_id;
  UserId taker_id;
  MatchId match_id;
  OrderId order_id;

  Quantity qty;
  Price price;
};
}  // namespace order_book_v1

#endif
