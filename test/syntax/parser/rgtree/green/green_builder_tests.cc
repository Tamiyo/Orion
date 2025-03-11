#include <gtest/gtest.h>

#include "syntax/parser/rgtree/green/green_builder.h"
#include "syntax/parser/rgtree/green/green_element.h"
#include "syntax/parser/rgtree/green/green_node.h"
#include "syntax/parser/rgtree/green/green_token.h"
#include "syntax/parser/syntax_kind.h"

namespace {
constexpr orion::syntax::SyntaxKind kTestSyntaxKind =
    orion::syntax::SyntaxKind::kError;

TEST(GreenBuilderTest, StartNode) {
  auto builder = orion::syntax::GreenBuilder();

  builder.StartNode(kTestSyntaxKind);

  EXPECT_EQ(1, builder.ParentsSize());
  EXPECT_EQ(0, builder.ChildrenSize());
}

TEST(GreenBuilderTest, FinishNode) {
  auto builder = orion::syntax::GreenBuilder();

  builder.StartNode(kTestSyntaxKind);
  builder.FinishNode();

  EXPECT_EQ(0, builder.ParentsSize());
  EXPECT_EQ(1, builder.ChildrenSize());
}

TEST(GreenBuilderTest, FinishNodeThrowsWhenNoNodes) {
  auto builder = orion::syntax::GreenBuilder();
  EXPECT_THROW({ builder.FinishNode(); }, std::invalid_argument);
}
}  // namespace