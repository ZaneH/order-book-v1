#include <gtest/gtest.h>
#include <hash.h>

namespace order_book_v1 {
TEST(Hash, SimpleNumberHash) {
  FixedWidth s1 = HASH_SEED;
  HashCombine(s1, 6);

  FixedWidth s2 = HASH_SEED;
  HashCombine(s2, 7);

  EXPECT_NE(s1, HASH_SEED);
  EXPECT_NE(s1, s2);
  EXPECT_NE(s1, 0);
}

TEST(Hash, SimpleEnum) {
  enum class Meal { kBreakfast = 0, kLunch, kDinner };

  FixedWidth s1 = HASH_SEED;
  HashCombine(s1, Meal::kLunch);

  FixedWidth s2 = HASH_SEED;
  HashCombine(s2, Meal::kDinner);

  EXPECT_NE(s1, HASH_SEED);
  EXPECT_NE(s1, s2);
  EXPECT_NE(s1, 0);
}
}  // namespace order_book_v1
