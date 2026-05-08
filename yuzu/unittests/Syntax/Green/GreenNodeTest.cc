#include "yuzu/Syntax/Green/Green.h"
#include "yuzu/Syntax/Green/GreenIterator.h"

#include <gmock/gmock.h>

namespace {
using yuzu::syntax::GreenChild;
using yuzu::syntax::GreenElement;
using yuzu::syntax::GreenNode;
using yuzu::syntax::GreenNodeData;
using yuzu::syntax::GreenToken;

const auto node = GreenNode::create(
    12, std::vector<GreenElement>{GreenToken(2, U"3"), GreenToken(3, U"-"),
                                  GreenToken(2, U"2")});

TEST(GreenNodeTest, GetKind) {
  // node has a kind of 12.
  EXPECT_EQ(12, node.getKind());
}

TEST(GreenNodeTest, GetWidth) {
  // node has a width of 3.
  EXPECT_EQ(3, node.getWidth());
}

TEST(GreenNodeTest, GetNumChildren) {
  // node has 3 children.
  EXPECT_EQ(3, node.getNumChildren());
}

TEST(GreenNodeTest, GetUseCount) {
  // node has 1 usage (this test).
  EXPECT_EQ(1, node.getUseCount());
}

TEST(GreenNodeTest, Equals) {
  const auto nodeCopy = GreenNode::create(
      12, std::vector<GreenElement>{GreenToken(2, U"3"), GreenToken(3, U"-"),
                                    GreenToken(2, U"2")});
  EXPECT_TRUE(node == nodeCopy);
}

TEST(GreenNodeTest, NotEquals) {
  const auto differentNode = GreenNode::create(12, std::vector<GreenElement>{});
  EXPECT_TRUE(node != differentNode);
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

TEST(GreenChildTest, GetRelativeOffset) {
  const auto it = node.getChildren().begin();
  EXPECT_EQ(0, it->relativeOffset);
}

TEST(GreenChildTest, GetElement) {
  const auto element = GreenElement(GreenToken(2, U"3"));
  const auto it = node.getChildren().begin();

  EXPECT_EQ(element, it->element);
}

TEST(GreenChildTest, Equals) {
  const auto element = GreenElement(GreenToken(2, U"3"));
  const auto child = GreenChild{.element = element, .relativeOffset = 0};
  const auto it = node.getChildren().begin();

  EXPECT_EQ(child, *it);
}

TEST(GreenChildTest, NotEquals) {
  const auto elemenet = GreenElement(GreenToken(2, U"3"));
  const auto child = GreenChild{.element = elemenet, .relativeOffset = 1};
  const auto it = node.getChildren().begin();

  EXPECT_NE(child, *it);
}

TEST(GreenChildTest, GreenChildSizeRequirements) {
  // RelativeOffset    = 8
  // GreenElement      = 24
  EXPECT_EQ(32, sizeof(GreenChild));
}
} // namespace
