#ifndef INCLUDE_TRADE_H_
#define INCLUDE_TRADE_H_

#include "types.h"

struct Trade {
  UserId maker_id;
  UserId taker_id;
  MatchId match_id;

  Quantity qty;
  Price price;
};

#endif
