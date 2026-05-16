#include "yuzu/Util/SideTable.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace {

using yuzu::util::SideTable;

enum class TestId : uint32_t {};

TEST(SideTableTest, GetOnUnboundIdReturnsNull) {
  SideTable<TestId, std::string> table;
  EXPECT_EQ(table.get(TestId{0}), nullptr);
  EXPECT_EQ(table.get(TestId{100}), nullptr);
}

TEST(SideTableTest, BindThenGetReturnsValue) {
  SideTable<TestId, std::string> table;
  table.bind(TestId{3}, "three");

  const auto *value = table.get(TestId{3});
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(*value, "three");
}

TEST(SideTableTest, RebindOverwrites) {
  SideTable<TestId, std::string> table;
  table.bind(TestId{1}, "first");
  table.bind(TestId{1}, "second");

  EXPECT_EQ(*table.get(TestId{1}), "second");
}

TEST(SideTableTest, SparseBindsLeaveOtherSlotsEmpty) {
  SideTable<TestId, int> table;
  table.bind(TestId{5}, 42);

  for (uint32_t i = 0; i < 5; ++i) {
    EXPECT_EQ(table.get(TestId{i}), nullptr) << "slot " << i;
  }
  EXPECT_EQ(*table.get(TestId{5}), 42);
  EXPECT_EQ(table.get(TestId{6}), nullptr);
}

TEST(SideTableTest, RawIntegerKeyWorks) {
  SideTable<uint32_t, std::string> table;
  table.bind(7, "seven");
  EXPECT_EQ(*table.get(7), "seven");
}

} // namespace
