#include "Syntax/Green/GreenIterator.h"

#include "Syntax/Green/Green.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

namespace {
using yuzu::syntax::GreenChild;
using yuzu::syntax::GreenChildren;
using yuzu::syntax::GreenElement;
using yuzu::syntax::GreenNode;
using yuzu::syntax::GreenToken;

const auto LeftNode =
    GreenElement(GreenNode::create(12, std::vector<GreenElement>{
                                           GreenElement(GreenToken(2, U"3")),
                                           GreenElement(GreenToken(3, U"-")),
                                           GreenElement(GreenToken(2, U"2")),
                                       }));

const auto EqualToken = GreenElement(GreenToken(9, U"="));

const auto RightNode =
    GreenElement(GreenNode::create(11, std::vector<GreenElement>{
                                           GreenElement(GreenToken(2, U"4")),
                                           GreenElement(GreenToken(3, U"+")),
                                           GreenElement(GreenToken(2, U"7")),
                                       }));

const auto Node = GreenNode::create(
    19, std::vector<GreenElement>{LeftNode, EqualToken, RightNode});

TEST(GreenNodeTest, GreenChildrenAndIterator) {
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

TEST(GreenNodeTest, GreenChildrenAndReverseIterator) {
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
