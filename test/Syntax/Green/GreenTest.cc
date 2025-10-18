#include "Syntax/Green/Green.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace {
using yuzu::syntax::GreenChild;
using yuzu::syntax::GreenElement;
using yuzu::syntax::GreenNode;
using yuzu::syntax::GreenNodeData;
using yuzu::syntax::GreenToken;
using yuzu::syntax::GreenTokenData;

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

  const size_t NumChildren = Node.getNumChildren();
  EXPECT_EQ(3, NumChildren);

  const GreenNode::Children GreenChildren = Node.getChildren();
  EXPECT_THAT(GreenChildren, testing::BeginEndDistanceIs(3));
  EXPECT_THAT(GreenChildren,
              testing::ElementsAre(
                  testing::Property(&GreenChild::getElement, LeftNode),
                  testing::Property(&GreenChild::getElement, EqualToken),
                  testing::Property(&GreenChild::getElement, RightNode)));
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

TEST(GreenChildTest, GreenChiildSizeRequirements) {
  // RelativeOffset    = 8
  // GreenElement      = 24
  EXPECT_EQ(32, sizeof(GreenChild));
}

TEST(GreenElementTest, GreenElementSizeRequirements) {
  // std::variant:
  //   shared_ptr:
  //     pointer    = 8
  //     ref_count  = 8
  // index          = 4
  // alignment      = 4
  EXPECT_EQ(24, sizeof(GreenElement));
}

} // namespace
