#include "yuzu/Util/LookupTable.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace {

using yuzu::util::LookupTable;

enum class TestId : uint32_t {};

TEST(LookupTableTest, GetOnUnboundIdReturnsNullopt) {
  LookupTable<TestId, std::string> table;
  EXPECT_FALSE(table.get(TestId{0}).has_value());
  EXPECT_FALSE(table.get(TestId{100}).has_value());
}

TEST(LookupTableTest, BindThenGetReturnsValue) {
  LookupTable<TestId, std::string> table;
  table.bind(TestId{3}, "three");

  const auto value = table.get(TestId{3});
  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(*value, "three");
}

TEST(LookupTableTest, RebindOverwrites) {
  LookupTable<TestId, std::string> table;
  table.bind(TestId{1}, "first");
  table.bind(TestId{1}, "second");

  EXPECT_EQ(*table.get(TestId{1}), "second");
}

TEST(LookupTableTest, SparseBindsLeaveOtherSlotsEmpty) {
  LookupTable<TestId, int> table;
  table.bind(TestId{5}, 42);

  for (uint32_t i = 0; i < 5; ++i) {
    EXPECT_FALSE(table.get(TestId{i}).has_value()) << "slot " << i;
  }
  EXPECT_EQ(*table.get(TestId{5}), 42);
  EXPECT_FALSE(table.get(TestId{6}).has_value());
}

TEST(LookupTableTest, RawIntegerKeyWorks) {
  LookupTable<uint32_t, std::string> table;
  table.bind(7, "seven");
  EXPECT_EQ(*table.get(7), "seven");
}

} // namespace
