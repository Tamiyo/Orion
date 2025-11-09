#include "yuzu/Syntax/Green/GreenBuilder.h"

#include "yuzu/Syntax/SyntaxKind.h"

#include <gtest/gtest.h>

namespace {
using yuzu::syntax::GreenBuilder;
using yuzu::syntax::SyntaxKind;

constexpr auto syntaxKind = 0;

TEST(GreenBuilderTest, StartNode) {
  auto builder = GreenBuilder();

  builder.startNode(syntaxKind);

  EXPECT_EQ(1, builder.getParentsSize());
  EXPECT_EQ(0, builder.getChildrenSize());
}

TEST(GreenBuilderTest, FinishNode) {
  auto builder = GreenBuilder();

  builder.startNode(syntaxKind);
  builder.finishNode();

  EXPECT_EQ(0, builder.getParentsSize());
  EXPECT_EQ(1, builder.getChildrenSize());
}
} // namespace
