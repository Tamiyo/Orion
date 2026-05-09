
#include "yuzu/Syntax/Green/Green.h"
#include "yuzu/Syntax/Syntax.h"
#include "yuzu/Syntax/SyntaxKind.h"

#include <gtest/gtest.h>

#include <vector>

namespace {
using yuzu::syntax::GreenElement;
using yuzu::syntax::GreenNode;
using yuzu::syntax::GreenToken;
using yuzu::syntax::SyntaxKind;
using yuzu::syntax::SyntaxNode;

TEST(SyntaxChildrenWithTokensTest, NoChildren) {
  const auto nodeEmpty = GreenNode::create(10, std::vector<GreenElement>());
  const auto root = SyntaxNode::createRoot(nodeEmpty);

  int count = 0;
  for (const auto &_ : root.getChildrenWithTokens()) {
    ++count;
  }
  EXPECT_EQ(0, count);
}

TEST(SyntaxChildrenWithTokensTest, TokensOnlyChildren) {
  const auto token1 = GreenToken(1, U"t1");
  const auto token2 = GreenToken(2, U"t2");
  const auto nodeWithTokens =
      GreenNode::create(11, std::vector<GreenElement>{token1, token2});
  const auto root = SyntaxNode::createRoot(nodeWithTokens);

  int tokenCount = 0;
  for (const auto &element : root.getChildrenWithTokens()) {
    EXPECT_NE(element.getIfToken(), nullptr);
    ++tokenCount;
  }
  EXPECT_EQ(2, tokenCount);
}

TEST(SyntaxChildrenWithTokensTest, NodesOnlyChildren) {
  const auto nodeEmpty = GreenNode::create(10, std::vector<GreenElement>());
  const auto nodeWithNodes =
      GreenNode::create(12, std::vector<GreenElement>{nodeEmpty});
  const auto root = SyntaxNode::createRoot(nodeWithNodes);

  auto children = root.getChildrenWithTokens();
  auto it = children.begin();

  const auto element = *it;
  EXPECT_NE(element.getIfNode(), nullptr);
  EXPECT_EQ(element.getNode().getGreen(), nodeEmpty);
  ++it;
  EXPECT_EQ(it, children.end());
}

TEST(SyntaxChildrenWithTokensTest, NodesAndTokensChildren) {
  const auto token1 = GreenToken(1, U"t1");
  const auto token2 = GreenToken(2, U"t2");
  const auto nodeEmpty = GreenNode::create(10, std::vector<GreenElement>());
  const auto nodeWithTokens =
      GreenNode::create(11, std::vector<GreenElement>{token1, token2});
  const auto nodeMixed = GreenNode::create(
      13, std::vector<GreenElement>{token1, nodeEmpty, token2, nodeWithTokens});
  const auto root = SyntaxNode::createRoot(nodeMixed);

  auto children = root.getChildrenWithTokens();
  auto it = children.begin();

  EXPECT_NE((*it++).getIfToken(), nullptr);

  const auto node1 = *it++;
  EXPECT_NE(node1.getIfNode(), nullptr);
  EXPECT_EQ(node1.getNode().getGreen(), nodeEmpty);

  EXPECT_NE((*it++).getIfToken(), nullptr);

  const auto node2 = *it++;
  EXPECT_NE(node2.getIfNode(), nullptr);
  EXPECT_EQ(node2.getNode().getGreen(), nodeWithTokens);

  EXPECT_EQ(it, children.end());
}

TEST(SyntaxChildrenWithTokensTest, ElementOffsets) {
  const auto manyChildren = GreenNode::create(
      0,
      std::vector<GreenElement>{
          GreenToken(1, U"("),
          GreenNode::create(4, std::vector<GreenElement>{GreenToken(3, U"*")}),
          GreenNode::create(5, std::vector<GreenElement>{GreenToken(2, U"4")}),
          GreenNode::create(5, std::vector<GreenElement>{GreenToken(2, U"3")}),
          GreenToken(5, U")"),
      });

  const auto root = SyntaxNode::createRoot(manyChildren);

  auto children = root.getChildrenWithTokens();
  auto it = children.begin();

  const auto element1 = *it++;
  EXPECT_NE(element1.getIfToken(), nullptr);
  EXPECT_EQ(0, element1.getToken().getOffset());

  const auto element2 = *it++;
  EXPECT_NE(element2.getIfNode(), nullptr);
  EXPECT_EQ(1, element2.getNode().getOffset());

  const auto element3 = *it++;
  EXPECT_NE(element3.getIfNode(), nullptr);
  EXPECT_EQ(2, element3.getNode().getOffset());

  const auto element4 = *it++;
  EXPECT_NE(element4.getIfNode(), nullptr);
  EXPECT_EQ(3, element4.getNode().getOffset());

  const auto element5 = *it++;
  EXPECT_NE(element5.getIfToken(), nullptr);
  EXPECT_EQ(4, element5.getToken().getOffset());

  EXPECT_EQ(it, children.end());
}

TEST(SyntaxChildrenWithTokensTest, Equals) {
  const auto token1 = GreenToken(1, U"t1");
  const auto token2 = GreenToken(2, U"t2");
  const auto nodeEmpty = GreenNode::create(10, std::vector<GreenElement>());
  const auto nodeWithTokens =
      GreenNode::create(11, std::vector<GreenElement>{token1, token2});
  const auto nodeMixed = GreenNode::create(
      13, std::vector<GreenElement>{token1, nodeEmpty, token2, nodeWithTokens});
  const auto root = SyntaxNode::createRoot(nodeMixed);
  const auto children = root.getChildrenWithTokens();

  EXPECT_EQ(children.begin(), children.begin());
}

TEST(SyntaxChildrenWithTokensTest, NotEquals) {
  const auto token1 = GreenToken(1, U"t1");
  const auto token2 = GreenToken(2, U"t2");
  const auto nodeEmpty = GreenNode::create(10, std::vector<GreenElement>());
  const auto nodeWithTokens =
      GreenNode::create(11, std::vector<GreenElement>{token1, token2});
  const auto nodeMixed = GreenNode::create(
      13, std::vector<GreenElement>{token1, nodeEmpty, token2, nodeWithTokens});
  const auto root = SyntaxNode::createRoot(nodeMixed);
  const auto children = root.getChildrenWithTokens();
  auto it = children.begin();

  EXPECT_NE(++it, children.begin());
}
} // namespace
