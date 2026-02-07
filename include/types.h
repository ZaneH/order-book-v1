#ifndef TYPES_H
#define TYPES_H

#include <cstdint>

enum class OrderSide { kBuy = 0, kSell };
enum class TimeInForce {
  kGoodTillCancel = 0,
  kImmediateOrCancel,
};

using Underlying = uint32_t;

struct OrderId {
  Underlying v_;
  bool operator==(const OrderId& other) const = default;
};

struct MatchId {
  Underlying v_;
  bool operator==(const MatchId& other) const = default;
};

struct UserId {
  Underlying v_;
  bool operator==(const UserId& other) const = default;
};

struct Ticks {
  Underlying v_;
  bool operator==(const Ticks& other) const = default;
};

struct Quantity {
  Underlying v_;
  bool operator==(const Quantity& other) const = default;
};

enum class OrderStatus {
  kAwaitingFill = 0,
  kPartialFill,
  kRejected,
};

#endif
