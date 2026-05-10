
#include "yuzu/Syntax/Green/Green.h"
#include "yuzu/Syntax/Detail/Syntax.h"
#include "yuzu/Syntax/SyntaxKind.h"

#include <gtest/gtest.h>

#include <vector>

namespace {
using yuzu::syntax::GreenElement;
using yuzu::syntax::GreenNode;
using yuzu::syntax::GreenToken;
using yuzu::syntax::SyntaxKind;
using yuzu::syntax::detail::SyntaxNode;

TEST(SyntaxChildrenTest, NoChildren) {
  const auto nodeEmpty = GreenNode::create(10, std::vector<GreenElement>());
  const auto root = SyntaxNode::createRoot(nodeEmpty);

  int count = 0;
  for (const auto &_ : root.getChildren()) {
    ++count;
  }
  EXPECT_EQ(0, count);
}

TEST(SyntaxChildrenTest, TokensOnlyChildren) {
  const auto token1 = GreenToken(1, U"t1");
  const auto token2 = GreenToken(2, U"t2");
  const auto nodeWithTokens =
      GreenNode::create(11, std::vector<GreenElement>{token1, token2});
  const auto root = SyntaxNode::createRoot(nodeWithTokens);

  int count = 0;
  for (const auto &_ : root.getChildren()) {
    ++count;
  }

  EXPECT_EQ(0, count);
}

TEST(SyntaxChildrenTest, NodesOnlyChildren) {
  const auto nodeEmpty = GreenNode::create(10, std::vector<GreenElement>());
  const auto nodeWithNodes =
      GreenNode::create(12, std::vector<GreenElement>{nodeEmpty});

  const auto root = SyntaxNode::createRoot(nodeWithNodes);

  auto children = root.getChildren();
  auto it = children.begin();

  EXPECT_EQ((*it++).getGreen(), nodeEmpty);
  EXPECT_EQ(it, children.end());
}

TEST(SyntaxChildrenTest, NodesAndTokensChildren) {
  const auto token1 = GreenToken(1, U"t1");
  const auto token2 = GreenToken(2, U"t2");
  const auto nodeEmpty = GreenNode::create(10, std::vector<GreenElement>());
  const auto nodeWithTokens =
      GreenNode::create(11, std::vector<GreenElement>{token1, token2});
  const auto nodeMixed = GreenNode::create(
      13, std::vector<GreenElement>{token1, nodeEmpty, token2, nodeWithTokens});
  const auto root = SyntaxNode::createRoot(nodeMixed);

  auto children = root.getChildren();
  auto it = children.begin();

  EXPECT_EQ((*it++).getGreen(), nodeEmpty);
  EXPECT_EQ((*it++).getGreen(), nodeWithTokens);
  EXPECT_EQ(it, children.end());
}

TEST(SyntaxChildrenTest, Equals) {
  const auto nodeEmpty = GreenNode::create(10, std::vector<GreenElement>());
  const auto nodeWithNodes =
      GreenNode::create(12, std::vector<GreenElement>{nodeEmpty});
  const auto root = SyntaxNode::createRoot(nodeWithNodes);
  const auto children = root.getChildren();

  EXPECT_EQ(children.begin(), children.begin());
}

TEST(SyntaxChildrenTest, NotEquals) {
  const auto nodeEmpty = GreenNode::create(10, std::vector<GreenElement>());
  const auto nodeWithNodes =
      GreenNode::create(12, std::vector<GreenElement>{nodeEmpty});
  const auto root = SyntaxNode::createRoot(nodeWithNodes);
  const auto children = root.getChildren();
  auto it = children.begin();

  EXPECT_NE(++it, children.begin());
}

TEST(SyntaxChildrenTest, NodesWithOffsets) {
  const auto manyChildren = GreenNode::create(
      0,
      std::vector<GreenElement>{
          GreenToken(1, U"("),
          GreenNode::create(4, std::vector<GreenElement>{GreenToken(3, U"*")}),
          GreenNode::create(5, std::vector<GreenElement>{GreenToken(2, U"4")}),
          GreenNode::create(5, std::vector<GreenElement>{GreenToken(2, U"3")}),
          GreenToken(1, U")"),
      });

  const auto root = SyntaxNode::createRoot(manyChildren);
  const auto children = root.getChildren();
  auto it = children.begin();

  EXPECT_EQ(1, (*it++).getOffset());
  EXPECT_EQ(2, (*it++).getOffset());
  EXPECT_EQ(3, (*it++).getOffset());
  EXPECT_EQ(it, children.end());
}
} // namespace
