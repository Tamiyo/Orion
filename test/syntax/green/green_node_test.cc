#include <gtest/gtest.h>

#include "syntax/green/green_node.h"


namespace {
using yuzu::syntax::GreenNode;
using yuzu::syntax::GreenNodeData;

TEST(GreenNodeTest, GreenNodeSizeRequirements) {
  // shared_ptr:
  //   pointer   = 8
  //   ref_count = 8
  EXPECT_EQ(16, sizeof(GreenNode));
}

TEST(GreenNodeTest, GreenNodeDataSizeRequirements) {
  // kind           = 2
  // alignment      = 6
  // width          = 8
  // std::vector    = 24
  EXPECT_EQ(40, sizeof(GreenNodeData));
}
};  // namespace
