#include <gtest/gtest.h>

#include "syntax/parser/rgtree/green/green_builder.h"
#include "syntax/parser/rgtree/green/green.h"
#include "syntax/syntax_kind.h"

namespace {
using orion::syntax::GreenBuilder;
using orion::syntax::SyntaxKind;

constexpr auto kTestSyntaxKind = SyntaxKind::kError;

TEST(GreenBuilderTest, StartNode) {
  auto builder = GreenBuilder();

  builder.StartNode(kTestSyntaxKind);

  EXPECT_EQ(1, builder.ParentsSize());
  EXPECT_EQ(0, builder.ChildrenSize());
}

TEST(GreenBuilderTest, FinishNode) {
  auto builder = GreenBuilder();

  builder.StartNode(kTestSyntaxKind);
  builder.FinishNode();

  EXPECT_EQ(0, builder.ParentsSize());
  EXPECT_EQ(1, builder.ChildrenSize());
}

TEST(GreenBuilderTest, FinishNodeThrowsWhenNoNodes) {
  auto builder = GreenBuilder();
  EXPECT_THROW({ builder.FinishNode(); }, std::invalid_argument);
}
}  // namespace
