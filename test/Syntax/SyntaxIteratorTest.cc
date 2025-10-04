#include "Syntax/SyntaxIterator.h"

#include "Syntax/Green/Green.h"
#include "Syntax/Syntax.h"
#include "Syntax/SyntaxKind.h"

#include <gtest/gtest.h>

#include <variant>
#include <vector>

namespace {
using yuzu::syntax::GreenElement;
using yuzu::syntax::GreenNode;
using yuzu::syntax::GreenToken;
using yuzu::syntax::SyntaxElement;
using yuzu::syntax::SyntaxKind;
using yuzu::syntax::SyntaxNode;
using yuzu::syntax::SyntaxToken;

TEST(SyntaxChildrenTest, NoChildren) {
  const auto NodeEmpty = GreenNode::create(10, std::vector<GreenElement>());
  const auto Root = SyntaxNode::createRoot(NodeEmpty);

  int Count = 0;
  for (auto Child : Root.getChildren()) {
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
  for (auto Child : Root.getChildren()) {
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
  for (auto Element : Root.getChildrenWithTokens()) {
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
  for (auto Element : Root.getChildrenWithTokens()) {
    EXPECT_TRUE(std::holds_alternative<SyntaxToken>(Element));
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
  // EXPECT_TRUE(std::holds_alternative<SyntaxNode>(Element));
  // EXPECT_EQ(std::get<SyntaxNode>(Element).getGreen(), NodeEmpty);
  // EXPECT_EQ(It, Children.end());
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

  EXPECT_TRUE(std::holds_alternative<SyntaxToken>(*It++));

  const auto Node1 = *It++;
  EXPECT_TRUE(std::holds_alternative<SyntaxNode>(Node1));
  EXPECT_EQ(std::get<SyntaxNode>(Node1).getGreen(), NodeEmpty);

  EXPECT_TRUE(std::holds_alternative<SyntaxToken>(*It++));

  const auto Node2 = *It++;
  EXPECT_TRUE(std::holds_alternative<SyntaxNode>(Node2));
  EXPECT_EQ(std::get<SyntaxNode>(Node2).getGreen(), NodeWithTokens);

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
          GreenToken(1, U")"),
      });

  const auto Root = SyntaxNode::createRoot(ManyChildren);
  auto Children = Root.getChildrenWithTokens();
  auto It = Children.begin();

  const auto Element1 = *It++;
  EXPECT_TRUE(std::holds_alternative<SyntaxToken>(Element1));
  EXPECT_EQ(0, std::get<SyntaxToken>(Element1).getOffset());

  const auto Element2 = *It++;
  EXPECT_TRUE(std::holds_alternative<SyntaxNode>(Element2));
  EXPECT_EQ(1, std::get<SyntaxNode>(Element2).getOffset());

  const auto Element3 = *It++;
  EXPECT_TRUE(std::holds_alternative<SyntaxNode>(Element3));
  EXPECT_EQ(2, std::get<SyntaxNode>(Element3).getOffset());

  const auto Element4 = *It++;
  EXPECT_TRUE(std::holds_alternative<SyntaxNode>(Element4));
  EXPECT_EQ(3, std::get<SyntaxNode>(Element4).getOffset());

  const auto Element5 = *It++;
  EXPECT_TRUE(std::holds_alternative<SyntaxToken>(Element5));
  EXPECT_EQ(4, std::get<SyntaxToken>(Element5).getOffset());

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
