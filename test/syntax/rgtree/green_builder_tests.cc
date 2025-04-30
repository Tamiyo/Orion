#include <gtest/gtest.h>

#include "syntax/rgtree/green.h"
#include "syntax/rgtree/green_builder.h"

namespace {
using yuzu::syntax::GreenBuilder;

enum class SyntaxKind : uint16_t { kError };

constexpr auto kTestSyntaxKind = SyntaxKind::kError;

TEST(GreenBuilderTest, StartNode) {
  auto builder = GreenBuilder<SyntaxKind>();

  builder.StartNode(kTestSyntaxKind);

  EXPECT_EQ(1, builder.ParentsSize());
  EXPECT_EQ(0, builder.ChildrenSize());
}

TEST(GreenBuilderTest, FinishNode) {
  auto builder = GreenBuilder<SyntaxKind>();

  builder.StartNode(kTestSyntaxKind);
  builder.FinishNode();

  EXPECT_EQ(0, builder.ParentsSize());
  EXPECT_EQ(1, builder.ChildrenSize());
}

TEST(GreenBuilderTest, FinishNodeThrowsWhenNoNodes) {
  auto builder = GreenBuilder<SyntaxKind>();
  EXPECT_THROW({ builder.FinishNode(); }, std::invalid_argument);
}
}  // namespace
