#include "Syntax/Syntax.h"

#include <gtest/gtest.h>

namespace {
using yuzu::syntax::SyntaxData;
using yuzu::syntax::SyntaxNode;
using yuzu::syntax::SyntaxToken;

TEST(SyntaxNodeTest, SyntaxNodeSizeRequirements) {
  // shared_ptr:
  //   pointer    = 8
  EXPECT_EQ(8, sizeof(SyntaxNode));
}

TEST(SyntaxTokenTest, SyntaxTokenSizeRequirements) {
  // shared_ptr:
  //   pointer    = 8
  EXPECT_EQ(8, sizeof(SyntaxToken));
}

TEST(SyntaxDataTest, SyntaxDataSizeRequirements) {
  // offset         = 8
  // parent         = 16
  // green          = 16
  EXPECT_EQ(40, sizeof(SyntaxData));
}
} // namespace
