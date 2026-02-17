#include "../include/types.h"

namespace order_book_v1 {
std::ostream& operator<<(std::ostream& os, OrderSide const side) {
  switch (side) {
    case OrderSide::kBuy: {
      os << "BUY";
      break;
    }
    case OrderSide::kSell: {
      os << "SELL";
      break;
    }
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, TimeInForce const tif) {
  switch (tif) {
    case TimeInForce::kGoodTillCancel: {
      os << "GTC";
      break;
    }
    case TimeInForce::kImmediateOrCancel: {
      os << "IOC";
      break;
    }
  }
  return os;
}
}  // namespace order_book_v1
