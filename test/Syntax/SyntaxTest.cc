#include "Syntax/Syntax.h"

#include "Syntax/Green/Green.h"
#include "Syntax/SyntaxKind.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

namespace {
using yuzu::syntax::SyntaxData;
using yuzu::syntax::SyntaxNode;
using yuzu::syntax::SyntaxToken;

TEST(SyntaxNodeTest, SyntaxNodeSizeRequirements) {
  // shared_ptr:
  //   pointer    = 8
  //   ref_count  = 8
  EXPECT_EQ(16, sizeof(SyntaxNode));
}

TEST(SyntaxTokenTest, SyntaxTokenSizeRequirements) {
  // shared_ptr:
  //   pointer    = 8
  //   ref_count  = 8
  EXPECT_EQ(16, sizeof(SyntaxToken));
}

TEST(SyntaxDataTest, SyntaxDataSizeRequirements) {
  // offset         = 8
  // parent         = 16
  // green          = 16
  EXPECT_EQ(40, sizeof(SyntaxData));
}
} // namespace
