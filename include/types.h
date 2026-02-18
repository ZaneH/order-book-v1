#ifndef INCLUDE_TYPES_H_
#define INCLUDE_TYPES_H_

#include <cstddef>
#include <cstdint>
#include <functional>
#include <ostream>

namespace order_book_v1 {
enum class EventType { kLimit = 0, kMarket, kCancel };
enum class OrderSide { kBuy = 0, kSell };
std::ostream& operator<<(std::ostream& os, OrderSide const side);

enum class TimeInForce {
  kGoodTillCancel = 0,
  kImmediateOrCancel,
};
std::ostream& operator<<(std::ostream& os, TimeInForce const tif);

using Underlying = uint32_t;

template <class Tag>
struct StrongId {
  Underlying v{};
  friend constexpr bool operator==(StrongId, StrongId) = default;
  friend std::ostream& operator<<(std::ostream& os, const StrongId& id) {
    os << id.v;
    return os;
  };
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

using OrderId = StrongId<OrderIdTag>;
using MatchId = StrongId<MatchIdTag>;
using UserId = StrongId<UserIdTag>;

template <class Tag>
struct StrongNum {
  Underlying v{};

  friend constexpr StrongNum operator+(StrongNum a, StrongNum b) {
    return a.v + b.v;
  }

  friend constexpr StrongNum operator+=(StrongNum& a, StrongNum b) {
    a.v = a.v + b.v;
    return a;
  }

  friend constexpr StrongNum operator-=(StrongNum<Tag>& a, StrongNum<Tag> b) {
    a.v = a.v - b.v;
    return a;
  }

  friend constexpr bool operator==(StrongNum, StrongNum) = default;
  friend constexpr auto operator<=>(StrongNum, StrongNum) = default;

  friend std::ostream& operator<<(std::ostream& os, const StrongNum& n) {
    os << n.v;
    return os;
  }
};

struct TicksTag {};
struct QuantityTag {};

using Quantity = StrongNum<QuantityTag>;
using Ticks = StrongNum<TicksTag>;
using Price = Ticks;

enum class OrderStatus {
  kAwaitingFill = 0,
  kPartialFill,
  kImmediateFill,
  kRejected,
};
}  // namespace order_book_v1

#endif
