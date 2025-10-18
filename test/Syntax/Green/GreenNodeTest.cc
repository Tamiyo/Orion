#include "Syntax/Green/Green.h"
#include "Syntax/Green/GreenIterator.h"

#include <gmock/gmock.h>

namespace {
using yuzu::syntax::GreenChild;
using yuzu::syntax::GreenNode;
using yuzu::syntax::GreenNodeData;

TEST(GreenNodeTest, GreenNodeSizeRequirements) {
  // shared_ptr:
  //   pointer    = 8
  //   ref_count  = 8
  EXPECT_EQ(16, sizeof(GreenNode));
}

TEST(GreenNodeTest, GreenNodeDataSizeRequirements) {
  // kind           = 2
  // alignment      = 6
  // width          = 8
  // size           = 16
  // pointer        = 16
  EXPECT_EQ(32, sizeof(GreenNodeData));
}

TEST(GreenNodeTest, GreenChildSizeRequirements) {
  // RelativeOffset    = 8
  // GreenElement      = 24
  EXPECT_EQ(32, sizeof(GreenChild));
}
} // namespace
