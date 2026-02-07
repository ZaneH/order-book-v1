#ifndef INCLUDE_TRADE_H_
#define INCLUDE_TRADE_H_

#include "types.h"

struct Trade {
  UserId maker_id_;
  UserId taker_id_;
  MatchId match_id_;

  Quantity qty_;
  Ticks price_;
};

#endif
