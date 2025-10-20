#include "Syntax/Green/Green.h"
#include "Syntax/Green/GreenIterator.h"

#include <gmock/gmock.h>

namespace {
using yuzu::syntax::GreenChild;
using yuzu::syntax::GreenElement;
using yuzu::syntax::GreenNode;
using yuzu::syntax::GreenNodeData;
using yuzu::syntax::GreenToken;

const auto Node = GreenNode::create(
    12, std::vector<GreenElement>{GreenElement(GreenToken(2, U"3")),
                                  GreenElement(GreenToken(3, U"-")),
                                  GreenElement(GreenToken(2, U"2"))});

TEST(GreenNodeTest, GetKind) {
  // Node has a kind of 12.
  EXPECT_EQ(12, Node.getKind());
}

TEST(GreenNodeTest, GetWidth) {
  // Node has a width of 3.
  EXPECT_EQ(3, Node.getWidth());
}

TEST(GreenNodeTest, GetNumChildren) {
  // Node has 3 children.
  EXPECT_EQ(3, Node.getNumChildren());
}

TEST(GreenNodeTest, GetUseCount) {
  // Node has 1 usage (this test).
  EXPECT_EQ(1, Node.getUseCount());
}

TEST(GreenNodeTest, Equals) {
  const auto NodeCopy = GreenNode::create(
      12, std::vector<GreenElement>{GreenElement(GreenToken(2, U"3")),
                                    GreenElement(GreenToken(3, U"-")),
                                    GreenElement(GreenToken(2, U"2"))});
  EXPECT_TRUE(Node == NodeCopy);
}

TEST(GreenNodeTest, NotEquals) {
  const auto DifferentNode = GreenNode::create(12, std::vector<GreenElement>{});
  EXPECT_TRUE(Node != DifferentNode);
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
  const auto It = Node.getChildren().begin();
  EXPECT_EQ(0, It->getRelativeOffset());
}

TEST(GreenChildTest, GetElement) {
  const auto Element = GreenElement(GreenToken(2, U"3"));
  const auto It = Node.getChildren().begin();

  EXPECT_EQ(Element, It->getElement());
}

TEST(GreenChildTest, Equals) {
  const auto Element = GreenElement(GreenToken(2, U"3"));
  const auto Child = GreenChild(0, Element);
  const auto It = Node.getChildren().begin();

  EXPECT_EQ(Child, *It);
}

TEST(GreenChildTest, NotEquals) {
  const auto Element = GreenElement(GreenToken(2, U"3"));
  const auto Child = GreenChild(1, Element);
  const auto It = Node.getChildren().begin();

  EXPECT_NE(Child, *It);
}

TEST(GreenChildTest, GreenChildSizeRequirements) {
  // RelativeOffset    = 8
  // GreenElement      = 24
  EXPECT_EQ(32, sizeof(GreenChild));
}
} // namespace
