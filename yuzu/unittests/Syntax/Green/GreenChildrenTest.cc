#include "yuzu/Syntax/Green/GreenIterator.h"

#include "yuzu/Syntax/Green/Green.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <cstddef>
#include <vector>

namespace {
using yuzu::syntax::GreenChild;
using yuzu::syntax::GreenChildren;
using yuzu::syntax::GreenElement;
using yuzu::syntax::GreenNode;
using yuzu::syntax::GreenToken;

const auto leftNode = GreenNode::create(
    12, std::vector<GreenElement>{GreenToken(2, U"3"), GreenToken(3, U"-"),
                                  GreenToken(2, U"2")});

const auto equalToken = GreenToken(9, U"=");

const auto rightNode = GreenNode::create(
    11, std::vector<GreenElement>{GreenToken(2, U"4"), GreenToken(3, U"+"),
                                  GreenToken(2, U"7")});

const auto node = GreenNode::create(
    19, std::vector<GreenElement>{leftNode, equalToken, rightNode});

TEST(GreenChildrenTest, IteratorSize) {
  EXPECT_EQ(3, node.getChildren().size());
}

TEST(GreenChildrenTest, IteratorsEqualTo) {
  EXPECT_EQ(node.getChildren().begin(), node.getChildren().begin());
  EXPECT_EQ(node.getChildren().end(), node.getChildren().end());
  EXPECT_EQ(node.getChildren().rbegin(), node.getChildren().rbegin());
  EXPECT_EQ(node.getChildren().rend(), node.getChildren().rend());
}

TEST(GreenChildrenTest, IteratorsNotEqualTo) {
  EXPECT_NE(node.getChildren().begin(), node.getChildren().end());
  EXPECT_NE(node.getChildren().end(), node.getChildren().begin());
  EXPECT_NE(node.getChildren().rbegin(), node.getChildren().rend());
  EXPECT_NE(node.getChildren().rend(), node.getChildren().rbegin());
}

TEST(GreenChildrenTest, IteratesOverAllGreenElements) {
  const size_t numChildren = node.getNumChildren();
  EXPECT_EQ(3, numChildren);

  const GreenChildren greenChildren = node.getChildren();
  EXPECT_THAT(greenChildren, testing::BeginEndDistanceIs(3));
  EXPECT_THAT(
      greenChildren,
      testing::ElementsAre(testing::Field(&GreenChild::element, leftNode),
                           testing::Field(&GreenChild::element, equalToken),
                           testing::Field(&GreenChild::element, rightNode)));
}

TEST(GreenChildrenTest, IteratesOverAllGreenElementsInReverse) {
  const size_t numChildren = node.getNumChildren();
  EXPECT_EQ(3, numChildren);

  const GreenChildren greenChildren = node.getChildren();
  const std::vector<GreenChild> ReversedGreenChildren(greenChildren.rbegin(),
                                                      greenChildren.rend());

  EXPECT_THAT(ReversedGreenChildren, testing::BeginEndDistanceIs(3));
  EXPECT_THAT(
      ReversedGreenChildren,
      testing::ElementsAre(testing::Field(&GreenChild::element, rightNode),
                           testing::Field(&GreenChild::element, equalToken),
                           testing::Field(&GreenChild::element, leftNode)));
}

} // namespace
