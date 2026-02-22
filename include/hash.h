#ifndef INCLUDE_HASH_H_
#define INCLUDE_HASH_H_

#include <cstdint>
#include <string_view>

namespace order_book_v1 {
using FixedWidth = uint64_t;
constexpr FixedWidth HASH_SEED = 0xDEADBEEF;

inline void HashCombine(FixedWidth& s, FixedWidth v) {
  s ^= v + 0x9e3779b9 + (s << 6) + (s >> 2);
}

inline void HashCombine(FixedWidth& s, const char* v) {
  if (v == nullptr) return;

  std::basic_string_view norm(v);
  for (auto c : norm) {
    HashCombine(s, static_cast<unsigned char>(c));
  }
}

template <typename E>
  requires std::is_enum_v<E>
inline void HashCombine(FixedWidth& s, E v) {
  using U = std::underlying_type_t<E>;
  HashCombine(s, static_cast<FixedWidth>(static_cast<U>(v)));
}
}  // namespace order_book_v1

#endif
