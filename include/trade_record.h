#ifndef TRADE_RECORD_H
#define TRADE_RECORD_H

#include "types.h"

struct TradeRecord {
  UserId maker_id_;
  UserId taker_id_;
  MatchId match_id_;
};

#endif
