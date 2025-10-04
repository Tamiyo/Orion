#include "Syntax/Green/Green.h"
#include "src/Syntax/Green/Green.h"

#include <gtest/gtest.h>

namespace {
using yuzu::syntax::GreenElement;
using yuzu::syntax::GreenNode;
using yuzu::syntax::GreenNodeData;
using yuzu::syntax::GreenToken;
using yuzu::syntax::GreenTokenData;

TEST(GreenElementTest, GreenElementSizeRequirements) {
  // std::variant:
  //   shared_ptr:
  //     pointer    = 8
  //     ref_count  = 8
  // index          = 4
  // alignment      = 4
  EXPECT_EQ(24, sizeof(GreenElement));
}

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

TEST(GreenNodeTest, GreenChildrenAndIterator) {
  const auto Node = GreenNode::create(
      4, std::vector<GreenElement>{
             GreenToken(2, U"hello"),
             GreenNode::create(
                 4, std::vector<GreenElement>{GreenToken(4, U"world")})});

  const GreenNode::Children Children = Node.getChildren();

  size_t Count = 0;
  for (const GreenElement &_ : Children) {
    Count += 1;
  }

  EXPECT_EQ(2, Count);
  EXPECT_EQ(2, Children.size());
}

TEST(GreenTokenTest, GreenTokenSizeRequirements) {
  // shared_ptr:
  //   pointer    = 8
  //   ref_count  = 8
  EXPECT_EQ(16, sizeof(GreenToken));
}

TEST(GreenTokenTest, GreenTokenDataSizeRequirements) {
  // kind                 = 2
  // alignment            = 6
  // std::u32string_view  = 32
  EXPECT_EQ(40, sizeof(GreenTokenData));
}
} // namespace
