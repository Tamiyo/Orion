#include "utils/views.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <iterator>
#include <optional>
#include <ranges>
#include <vector>

namespace {
const std::vector<int> kTestContainer = {1, 2, 3};

TEST(ViewsTest, FilterMapWithValue) {
  auto result = kTestContainer | yuzu::utils::FilterMap([](int x) {
                  return (x <= 1) ? std::make_optional(x) : std::nullopt;
                });

  const auto distance = std::ranges::distance(result.begin(), result.end());
  EXPECT_EQ(1, distance);
}

TEST(ViewsTest, FilterMapWithoutValue) {
  auto result = kTestContainer | yuzu::utils::FilterMap([](int x) {
                  return (x == 7) ? std::make_optional(x) : std::nullopt;
                });

  const auto distance = std::ranges::distance(result.begin(), result.end());
  EXPECT_EQ(0, distance);
}

TEST(ViewsTest, FilterWithValues) {
  auto result =
      kTestContainer | yuzu::utils::Filter([](int x) { return x < 2; });

  const auto distance = std::ranges::distance(result.begin(), result.end());
  EXPECT_EQ(1, distance);
}

TEST(ViewsTest, FilterWithoutValues) {
  auto result =
      kTestContainer | yuzu::utils::Filter([](int x) { return x > 7; });

  const auto distance = std::ranges::distance(result.begin(), result.end());
  EXPECT_EQ(0, distance);
}

TEST(ViewsTest, FindWithValue) {
  const std::optional<int> result =
      kTestContainer | yuzu::utils::Find([](int x) { return x == 3; });

  ASSERT_EQ(true, result.has_value());
  EXPECT_EQ(3, result.value());
}

TEST(ViewsTest, FindWithoutValue) {
  const std::optional<int> result =
      kTestContainer | yuzu::utils::Find([](int x) { return x == 7; });

  EXPECT_EQ(std::nullopt, result);
}

TEST(ViewsTest, NthItem) {
  const std::optional<int> result = kTestContainer | yuzu::utils::Nth(2);

  EXPECT_EQ(3, result);
}

TEST(ViewsTest, NthItemOutOfBounds) {
  const std::optional<int> result = kTestContainer | yuzu::utils::Nth(100);

  EXPECT_EQ(std::nullopt, result);
}
}  // namespace
