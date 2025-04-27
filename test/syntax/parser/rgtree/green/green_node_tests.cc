#include <gtest/gtest.h>

#include <cstdint>

#include "syntax/parser/rgtree/green/green.h"

namespace {
using yuzu::syntax::GreenNode;
using yuzu::syntax::GreenNodeData;

enum class SyntaxKind : uint16_t {};

TEST(GreenNodeTest, GreenNodeSizeRequirements) {
  // shared_ptr:
  //   pointer   = 8
  //   ref_count = 8
  EXPECT_EQ(16, sizeof(GreenNode<SyntaxKind>));
}

TEST(GreenNodeTest, GreenNodeDataSizeRequirements) {
  // kind           = 2
  // alignment      = 6
  // width          = 8
  // std::vector    = 24
  EXPECT_EQ(40, sizeof(GreenNodeData<SyntaxKind>));
}
};  // namespace
