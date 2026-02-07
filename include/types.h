#ifndef INCLUDE_TYPES_H_
#define INCLUDE_TYPES_H_

#include <cstdint>

enum class OrderSide { kBuy = 0, kSell };
enum class TimeInForce {
  kGoodTillCancel = 0,
  kImmediateOrCancel,
};

using Underlying = uint32_t;

struct OrderId {
  Underlying v;
  bool operator==(const OrderId& other) const = default;
};

struct MatchId {
  Underlying v;
  bool operator==(const MatchId& other) const = default;
};

struct UserId {
  Underlying v;
  bool operator==(const UserId& other) const = default;
};

struct Ticks {
  Underlying v;
  bool operator==(const Ticks& other) const = default;
};

using Price = Ticks;

struct Quantity {
  Underlying v;
  bool operator==(const Quantity& other) const = default;
};

enum class OrderStatus {
  kAwaitingFill = 0,
  kPartialFill,
  kRejected,
};

#endif
