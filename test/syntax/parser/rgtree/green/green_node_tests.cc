#include <gtest/gtest.h>

#include "syntax/parser/rgtree/green/green_element.h"
#include "syntax/parser/rgtree/green/green_node.h"

namespace {
TEST(GreenNodeTest, GreenNode_SizeRequirements) {
  // shared_ptr:
  //   pointer   = 8
  //   ref_count = 8
  EXPECT_EQ(16, sizeof(orion::syntax::GreenNode));
}

TEST(GreenNodeTest, GreenNodeData_SizeRequirements) {
  // kind           = 2
  // alignment      = 6
  // width          = 8
  // std::vector    = 24
  EXPECT_EQ(40, sizeof(orion::syntax::GreenNodeData));
}
};  // namespace
