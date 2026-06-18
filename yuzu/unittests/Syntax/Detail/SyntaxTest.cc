#include "yuzu/Syntax/Detail/Syntax.h"

#include <gtest/gtest.h>

namespace {
using yuzu::syntax::detail::SyntaxData;
using yuzu::syntax::detail::SyntaxElement;
using yuzu::syntax::detail::SyntaxNode;
using yuzu::syntax::detail::SyntaxToken;

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
  // green          = 24
  // parent         =  8
  // offset         =  4  (uint32)
  // index          =  4  (uint32)
  // rc             =  4  (uint32)
  // padding        =  4  (align to 8)
  EXPECT_EQ(48, sizeof(SyntaxData));
}
} // namespace
