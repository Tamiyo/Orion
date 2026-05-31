#include "yuzu/Util/StringInterner.h"

#include <gtest/gtest.h>

#include <string>

namespace {

using yuzu::util::StringInterner;

TEST(StringInternerTest, EqualContentSharesOnePointer) {
  // The whole point: equal strings collapse to one canonical buffer, so
  // interned views can be compared by `data()` pointer.
  StringInterner interner;
  const auto a = interner.intern(U"employee");
  const auto b = interner.intern(U"employee");
  EXPECT_EQ(a.data(), b.data());
  EXPECT_EQ(a, U"employee");
}

TEST(StringInternerTest, DistinctContentGetsDistinctStorage) {
  StringInterner interner;
  const auto a = interner.intern(U"id");
  const auto b = interner.intern(U"tenure");
  EXPECT_NE(a.data(), b.data());
  EXPECT_EQ(a, U"id");
  EXPECT_EQ(b, U"tenure");
}

TEST(StringInternerTest, ResultOutlivesCallerBuffer) {
  // Interned storage is owned by the interner, so it stays valid after the
  // source string is destroyed/mutated.
  StringInterner interner;
  std::u32string_view interned;
  {
    std::u32string transient = U"ephemeral";
    interned = interner.intern(transient);
    transient = U"clobbered";
  }
  EXPECT_EQ(interned, U"ephemeral");
}

TEST(StringInternerTest, EmptyStringInternsToEmptyView) {
  StringInterner interner;
  const auto empty = interner.intern(U"");
  EXPECT_TRUE(empty.empty());
  // Interning empty repeatedly is stable and allocation-free.
  EXPECT_TRUE(interner.intern(std::u32string_view{}).empty());
}

} // namespace
