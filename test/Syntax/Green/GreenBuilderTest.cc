#include "Syntax/Green/GreenBuilder.h"

#include "Syntax/Green/Green.h"
#include "Syntax/SyntaxKind.h"

#include <gtest/gtest.h>

namespace {
using yuzu::syntax::GreenBuilder;
using yuzu::syntax::SyntaxKind;

constexpr auto kTestSyntaxKind = 0;

TEST(GreenBuilderTest, StartNode) {
  auto Builder = GreenBuilder();

  Builder.startNode(kTestSyntaxKind);

  EXPECT_EQ(1, Builder.getParentsSize());
  EXPECT_EQ(0, Builder.getChildrenSize());
}

TEST(GreenBuilderTest, FinishNode) {
  auto Builder = GreenBuilder();

  Builder.startNode(kTestSyntaxKind);
  Builder.finishNode();

  EXPECT_EQ(0, Builder.getParentsSize());
  EXPECT_EQ(1, Builder.getChildrenSize());
}
} // namespace
