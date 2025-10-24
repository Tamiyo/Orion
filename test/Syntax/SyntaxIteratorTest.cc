#include "Syntax/SyntaxIterator.h"

#include "Syntax/Green/Green.h"
#include "Syntax/Syntax.h"
#include "Syntax/SyntaxKind.h"

#include <gtest/gtest.h>

#include <vector>

namespace {
using yuzu::syntax::GreenElement;
using yuzu::syntax::GreenNode;
using yuzu::syntax::GreenToken;
using yuzu::syntax::SyntaxKind;
using yuzu::syntax::SyntaxNode;

TEST(SyntaxChildrenTest, NoChildren) {
  const auto NodeEmpty = GreenNode::create(10, std::vector<GreenElement>());
  const auto Root = SyntaxNode::createRoot(NodeEmpty);

  int Count = 0;
  for (const auto &_ : Root.getChildren()) {
    ++Count;
  }
  EXPECT_EQ(0, Count);
}

TEST(SyntaxChildrenTest, TokensOnlyChildren) {
  const auto Token1 = GreenToken(1, U"t1");
  const auto Token2 = GreenToken(2, U"t2");
  const auto NodeWithTokens =
      GreenNode::create(11, std::vector<GreenElement>{Token1, Token2});
  const auto Root = SyntaxNode::createRoot(NodeWithTokens);

  int Count = 0;
  for (const auto &_ : Root.getChildren()) {
    ++Count;
  }
  EXPECT_EQ(0, Count);
}

TEST(SyntaxChildrenTest, NodesOnlyChildren) {
  const auto NodeEmpty = GreenNode::create(10, std::vector<GreenElement>());
  const auto NodeWithNodes =
      GreenNode::create(12, std::vector<GreenElement>{NodeEmpty});

  const auto Root = SyntaxNode::createRoot(NodeWithNodes);

  auto Children = Root.getChildren();
  auto It = Children.begin();

  EXPECT_EQ((*It++).getGreen(), NodeEmpty);
  EXPECT_EQ(It, Children.end());
}

TEST(SyntaxChildrenTest, NodesAndTokensChildren) {
  const auto Token1 = GreenToken(1, U"t1");
  const auto Token2 = GreenToken(2, U"t2");
  const auto NodeEmpty = GreenNode::create(10, std::vector<GreenElement>());
  const auto NodeWithTokens =
      GreenNode::create(11, std::vector<GreenElement>{Token1, Token2});
  const auto NodeMixed = GreenNode::create(
      13, std::vector<GreenElement>{Token1, NodeEmpty, Token2, NodeWithTokens});
  const auto Root = SyntaxNode::createRoot(NodeMixed);

  auto Children = Root.getChildren();
  auto It = Children.begin();

  EXPECT_EQ((*It++).getGreen(), NodeEmpty);
  EXPECT_EQ((*It++).getGreen(), NodeWithTokens);
  EXPECT_EQ(It, Children.end());
}

TEST(SyntaxChildrenTest, Equals) {
  const auto NodeEmpty = GreenNode::create(10, std::vector<GreenElement>());
  const auto NodeWithNodes =
      GreenNode::create(12, std::vector<GreenElement>{NodeEmpty});
  const auto Root = SyntaxNode::createRoot(NodeWithNodes);
  auto Children = Root.getChildren();

  EXPECT_EQ(Children.begin(), Children.begin());
}

TEST(SyntaxChildrenTest, NotEquals) {
  const auto NodeEmpty = GreenNode::create(10, std::vector<GreenElement>());
  const auto NodeWithNodes =
      GreenNode::create(12, std::vector<GreenElement>{NodeEmpty});
  const auto Root = SyntaxNode::createRoot(NodeWithNodes);
  auto Children = Root.getChildren();
  auto It = Children.begin();

  EXPECT_NE(++It, Children.begin());
}

TEST(SyntaxChildrenTest, NodesWithOffsets) {
  const auto ManyChildren = GreenNode::create(
      0,
      std::vector<GreenElement>{
          GreenToken(1, U"("),
          GreenNode::create(4, std::vector<GreenElement>{GreenToken(3, U"*")}),
          GreenNode::create(5, std::vector<GreenElement>{GreenToken(2, U"4")}),
          GreenNode::create(5, std::vector<GreenElement>{GreenToken(2, U"3")}),
          GreenToken(1, U")"),
      });

  const auto Root = SyntaxNode::createRoot(ManyChildren);
  auto Children = Root.getChildren();
  auto It = Children.begin();

  EXPECT_EQ(1, (*It++).getOffset());
  EXPECT_EQ(2, (*It++).getOffset());
  EXPECT_EQ(3, (*It++).getOffset());
  EXPECT_EQ(It, Children.end());
}

TEST(SyntaxChildrenWithTokensTest, NoChildren) {
  const auto NodeEmpty = GreenNode::create(10, std::vector<GreenElement>());
  const auto Root = SyntaxNode::createRoot(NodeEmpty);

  int Count = 0;
  for (const auto &_ : Root.getChildrenWithTokens()) {
    ++Count;
  }
  EXPECT_EQ(0, Count);
}

TEST(SyntaxChildrenWithTokensTest, TokensOnlyChildren) {
  const auto Token1 = GreenToken(1, U"t1");
  const auto Token2 = GreenToken(2, U"t2");
  const auto NodeWithTokens =
      GreenNode::create(11, std::vector<GreenElement>{Token1, Token2});
  const auto Root = SyntaxNode::createRoot(NodeWithTokens);

  int TokenCount = 0;
  for (const auto &Element : Root.getChildrenWithTokens()) {
    EXPECT_NE(Element.getIfToken(), nullptr);
    ++TokenCount;
  }
  EXPECT_EQ(2, TokenCount);
}

TEST(SyntaxChildrenWithTokensTest, NodesOnlyChildren) {
  const auto NodeEmpty = GreenNode::create(10, std::vector<GreenElement>());
  const auto NodeWithNodes =
      GreenNode::create(12, std::vector<GreenElement>{NodeEmpty});
  const auto Root = SyntaxNode::createRoot(NodeWithNodes);

  auto Children = Root.getChildrenWithTokens();
  auto It = Children.begin();

  const auto Element = *It;
  EXPECT_NE(Element.getIfNode(), nullptr);
  EXPECT_EQ(Element.getNode().getGreen(), NodeEmpty);
  ++It;
  EXPECT_EQ(It, Children.end());
}

TEST(SyntaxChildrenWithTokensTest, NodesAndTokensChildren) {
  const auto Token1 = GreenToken(1, U"t1");
  const auto Token2 = GreenToken(2, U"t2");
  const auto NodeEmpty = GreenNode::create(10, std::vector<GreenElement>());
  const auto NodeWithTokens =
      GreenNode::create(11, std::vector<GreenElement>{Token1, Token2});
  const auto NodeMixed = GreenNode::create(
      13, std::vector<GreenElement>{Token1, NodeEmpty, Token2, NodeWithTokens});
  const auto Root = SyntaxNode::createRoot(NodeMixed);

  auto Children = Root.getChildrenWithTokens();
  auto It = Children.begin();

  EXPECT_NE((*It++).getIfToken(), nullptr);

  const auto Node1 = *It++;
  EXPECT_NE(Node1.getIfNode(), nullptr);
  EXPECT_EQ(Node1.getNode().getGreen(), NodeEmpty);

  EXPECT_NE((*It++).getIfToken(), nullptr);

  const auto Node2 = *It++;
  EXPECT_NE(Node2.getIfNode(), nullptr);
  EXPECT_EQ(Node2.getNode().getGreen(), NodeWithTokens);

  EXPECT_EQ(It, Children.end());
}

TEST(SyntaxChildrenWithTokensTest, ElementOffsets) {
  const auto ManyChildren = GreenNode::create(
      0,
      std::vector<GreenElement>{
          GreenToken(1, U"("),
          GreenNode::create(4, std::vector<GreenElement>{GreenToken(3, U"*")}),
          GreenNode::create(5, std::vector<GreenElement>{GreenToken(2, U"4")}),
          GreenNode::create(5, std::vector<GreenElement>{GreenToken(2, U"3")}),
          GreenToken(5, U")"),
      });

  const auto Root = SyntaxNode::createRoot(ManyChildren);

  auto Children = Root.getChildrenWithTokens();
  auto It = Children.begin();

  const auto Element1 = *It++;
  EXPECT_NE(Element1.getIfToken(), nullptr);
  EXPECT_EQ(0, Element1.getToken().getOffset());

  const auto Element2 = *It++;
  EXPECT_NE(Element2.getIfNode(), nullptr);
  EXPECT_EQ(1, Element2.getNode().getOffset());

  const auto Element3 = *It++;
  EXPECT_NE(Element3.getIfNode(), nullptr);
  EXPECT_EQ(2, Element3.getNode().getOffset());

  const auto Element4 = *It++;
  EXPECT_NE(Element4.getIfNode(), nullptr);
  EXPECT_EQ(3, Element4.getNode().getOffset());

  const auto Element5 = *It++;
  EXPECT_NE(Element5.getIfToken(), nullptr);
  EXPECT_EQ(4, Element5.getToken().getOffset());

  EXPECT_EQ(It, Children.end());
}

TEST(SyntaxChildrenWithTokensTest, Equals) {
  const auto Token1 = GreenToken(1, U"t1");
  const auto Token2 = GreenToken(2, U"t2");
  const auto NodeEmpty = GreenNode::create(10, std::vector<GreenElement>());
  const auto NodeWithTokens =
      GreenNode::create(11, std::vector<GreenElement>{Token1, Token2});
  const auto NodeMixed = GreenNode::create(
      13, std::vector<GreenElement>{Token1, NodeEmpty, Token2, NodeWithTokens});
  const auto Root = SyntaxNode::createRoot(NodeMixed);
  auto Children = Root.getChildrenWithTokens();

  EXPECT_EQ(Children.begin(), Children.begin());
}

TEST(SyntaxChildrenWithTokensTest, NotEquals) {
  const auto Token1 = GreenToken(1, U"t1");
  const auto Token2 = GreenToken(2, U"t2");
  const auto NodeEmpty = GreenNode::create(10, std::vector<GreenElement>());
  const auto NodeWithTokens =
      GreenNode::create(11, std::vector<GreenElement>{Token1, Token2});
  const auto NodeMixed = GreenNode::create(
      13, std::vector<GreenElement>{Token1, NodeEmpty, Token2, NodeWithTokens});
  const auto Root = SyntaxNode::createRoot(NodeMixed);
  auto Children = Root.getChildrenWithTokens();
  auto It = Children.begin();

  EXPECT_NE(++It, Children.begin());
}

} // namespace
