#include "yuzu/Syntax/Green/GreenIterator.h"

#include "yuzu/Syntax/Green/Green.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <vector>

namespace {
using yuzu::syntax::GreenChild;
using yuzu::syntax::GreenChildren;
using yuzu::syntax::GreenElement;
using yuzu::syntax::GreenNode;
using yuzu::syntax::GreenToken;

const auto LeftNode = GreenNode::create(
    12, std::vector<GreenElement>{GreenToken(2, U"3"), GreenToken(3, U"-"),
                                  GreenToken(2, U"2")});

const auto EqualToken = GreenToken(9, U"=");

const auto RightNode = GreenNode::create(
    11, std::vector<GreenElement>{GreenToken(2, U"4"), GreenToken(3, U"+"),
                                  GreenToken(2, U"7")});

const auto Node = GreenNode::create(
    19, std::vector<GreenElement>{LeftNode, EqualToken, RightNode});

TEST(GreenChildrenTest, IteratorSize) {
  EXPECT_EQ(3, Node.getChildren().size());
}

TEST(GreenChildrenTest, IteratorsEqualTo) {
  EXPECT_EQ(Node.getChildren().begin(), Node.getChildren().begin());
  EXPECT_EQ(Node.getChildren().end(), Node.getChildren().end());
  EXPECT_EQ(Node.getChildren().rbegin(), Node.getChildren().rbegin());
  EXPECT_EQ(Node.getChildren().rend(), Node.getChildren().rend());
}

TEST(GreenChildrenTest, IteratorsNotEqualTo) {
  EXPECT_NE(Node.getChildren().begin(), Node.getChildren().end());
  EXPECT_NE(Node.getChildren().end(), Node.getChildren().begin());
  EXPECT_NE(Node.getChildren().rbegin(), Node.getChildren().rend());
  EXPECT_NE(Node.getChildren().rend(), Node.getChildren().rbegin());
}

TEST(GreenChildrenTest, IteratesOverAllGreenElements) {
  const size_t NumChildren = Node.getNumChildren();
  EXPECT_EQ(3, NumChildren);

  const GreenChildren GreenChildren = Node.getChildren();
  EXPECT_THAT(GreenChildren, testing::BeginEndDistanceIs(3));
  EXPECT_THAT(GreenChildren,
              testing::ElementsAre(
                  testing::Property(&GreenChild::getElement, LeftNode),
                  testing::Property(&GreenChild::getElement, EqualToken),
                  testing::Property(&GreenChild::getElement, RightNode)));
}

TEST(GreenChildrenTest, IteratesOverAllGreenElementsInReverse) {
  const size_t NumChildren = Node.getNumChildren();
  EXPECT_EQ(3, NumChildren);

  const GreenChildren GreenChildren = Node.getChildren();
  const std::vector<GreenChild> ReversedGreenChildren(GreenChildren.rbegin(),
                                                      GreenChildren.rend());

  EXPECT_THAT(ReversedGreenChildren, testing::BeginEndDistanceIs(3));
  EXPECT_THAT(ReversedGreenChildren,
              testing::ElementsAre(
                  testing::Property(&GreenChild::getElement, RightNode),
                  testing::Property(&GreenChild::getElement, EqualToken),
                  testing::Property(&GreenChild::getElement, LeftNode)));
}

} // namespace
