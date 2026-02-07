#ifndef INCLUDE_TYPES_H_
#define INCLUDE_TYPES_H_

#include <cstddef>
#include <cstdint>
#include <functional>

enum class OrderSide { kBuy = 0, kSell };
enum class TimeInForce {
  kGoodTillCancel = 0,
  kImmediateOrCancel,
};

using Underlying = uint32_t;

template <class Tag>
struct StrongId {
  Underlying v{};
  bool operator==(const StrongId&) const = default;
};

template <class Tag>
struct StrongIdHash {
  size_t operator()(const StrongId<Tag>& x) const noexcept {
    return std::hash<Underlying>{}(x.v);
  }
};

struct OrderIdTag {};
struct MatchIdTag {};
struct UserIdTag {};
struct TicksTag {};
struct QuantityTag {};

using OrderId = StrongId<OrderIdTag>;
using MatchId = StrongId<MatchIdTag>;
using UserId = StrongId<UserIdTag>;
using Ticks = StrongId<TicksTag>;
using Quantity = StrongId<QuantityTag>;
using Price = Ticks;

enum class OrderStatus {
  kAwaitingFill = 0,
  kPartialFill,
  kRejected,
};

#endif
