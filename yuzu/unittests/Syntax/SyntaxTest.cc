#include "yuzu/Syntax/Syntax.h"

#include "gtest/gtest.h"

namespace {
using yuzu::syntax::SyntaxData;
using yuzu::syntax::SyntaxElement;
using yuzu::syntax::SyntaxNode;
using yuzu::syntax::SyntaxToken;

TEST(SyntaxNodeTest, SyntaxNodeSizeRequirements) {
  //   shared_ptr:
  //     pointer    = 8
  //     ref_count  = 8
  EXPECT_EQ(8, sizeof(SyntaxNode));
}

TEST(SyntaxTokenTest, SyntaxTokenSizeRequirements) {
  //   shared_ptr:
  //     pointer    = 8
  EXPECT_EQ(8, sizeof(SyntaxToken));
}

TEST(SyntaxElementTest, SyntaxElementSizeRequirements) {
  // std::variant:
  //   shared_ptr:
  //     pointer    = 8
  // index          = 4
  // alignment      = 4
  EXPECT_EQ(16, sizeof(SyntaxElement));
}

TEST(SyntaxDataTest, SyntaxDataSizeRequirements) {
  // offset         = 8
  // index          = 8
  // parent         = 16
  // green          = 16
  EXPECT_EQ(56, sizeof(SyntaxData));
}
} // namespace
