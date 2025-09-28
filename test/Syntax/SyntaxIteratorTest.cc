#include "Syntax/SyntaxIterator.h"

#include "Syntax/Green/Green.h"
#include "Syntax/Syntax.h"
#include "Syntax/SyntaxKind.h"

#include <gtest/gtest.h>

#include <memory>
#include <variant>
#include <vector>

namespace {
using yuzu::syntax::GreenElement;
using yuzu::syntax::GreenNode;
using yuzu::syntax::GreenToken;
using yuzu::syntax::SyntaxKind;
using yuzu::syntax::SyntaxNode;
using yuzu::syntax::SyntaxToken;
using yuzu::syntax::SyntaxElement;
using yuzu::syntax::SyntaxChildren;
using yuzu::syntax::SyntaxChildrenWithTokens;

const auto Token1 = GreenToken({.Value = 1}, U"token");
const auto Token2 = GreenToken({.Value = 1}, U"token");
const auto NodeEmpty = GreenNode({.Value = 10}, {});
const auto NodeWithTokens = GreenNode(
    {.Value = 11}, {GreenElement(Token1), GreenElement(Token2)});
const auto NodeWithNodes =
    GreenNode({.Value = 12}, {GreenElement(NodeEmpty)});
const auto NodeMixed = GreenNode(
    {.Value = 13},
    {GreenElement(Token1), GreenElement(NodeEmpty),
     GreenElement(Token2), GreenElement(NodeWithTokens)});

TEST(SyntaxChildrenTest, NoChildren) {
  const auto Root = SyntaxNode::createRoot(NodeEmpty);

  int Count = 0;
  for (auto Child : SyntaxChildren(Root)) {
    ++Count;
  }
  EXPECT_EQ(0, Count);
}

TEST(SyntaxChildrenTest, TokensOnlyChildren) {
  const auto Root = SyntaxNode::createRoot(NodeWithTokens);

  int Count = 0;
  for (auto Child : SyntaxChildren(Root)) {
    ++Count;
  }
  EXPECT_EQ(0, Count);
}

TEST(SyntaxChildrenTest, NodesOnlyChildren) {
  const auto Root = SyntaxNode::createRoot(NodeWithNodes);

  auto Children = SyntaxChildren(Root);
  auto It = Children.begin();

  EXPECT_EQ((*It++).getGreen(), NodeEmpty);
  EXPECT_EQ(It, Children.end());
}

TEST(SyntaxChildrenTest, NodesAndTokensChildren) {
  const auto Root = SyntaxNode::createRoot(NodeMixed);

  auto Children = SyntaxChildren(Root);
  auto It = Children.begin();

  EXPECT_EQ((*It++).getGreen(), NodeEmpty);
  EXPECT_EQ((*It++).getGreen(), NodeWithTokens);
  EXPECT_EQ(It, Children.end());
}

TEST(SyntaxChildrenTest, Equals) {
  const auto Root = SyntaxNode::createRoot(NodeWithNodes);
  auto Children = SyntaxChildren(Root);

  EXPECT_EQ(Children.begin(), Children.begin());
}

TEST(SyntaxChildrenTest, NotEquals) {
  const auto Root = SyntaxNode::createRoot(NodeWithNodes);
  auto Children = SyntaxChildren(Root);
  auto It = Children.begin();

  EXPECT_NE(++It, Children.begin());
}

TEST(SyntaxChildrenTest, NodesWithOffsets) {
  const auto ManyChildren = GreenNode(
      {.Value = 0},
      {
          GreenElement(GreenToken({.Value = 1}, U"(")),
          GreenElement(GreenNode(
              {.Value = 4},
              {GreenElement(GreenToken({.Value = 3}, U"*"))})),
          GreenElement(GreenNode(
              {.Value = 5},
              {GreenElement(GreenToken({.Value = 2}, U"4"))})),
          GreenElement(GreenNode(
              {.Value = 5},
              {GreenElement(GreenToken({.Value = 2}, U"3"))})),
          GreenElement(GreenToken({.Value = 1}, U")")),
      });

  const auto Root = SyntaxNode::createRoot(ManyChildren);
  auto Children = SyntaxChildren(Root);
  auto It = Children.begin();

  EXPECT_EQ(1, (*It++).getOffset());
  EXPECT_EQ(2, (*It++).getOffset());
  EXPECT_EQ(3, (*It++).getOffset());
  EXPECT_EQ(It, Children.end());
}

TEST(SyntaxChildrenWithTokensTest, NoChildren) {
  const auto Root = SyntaxNode::createRoot(NodeEmpty);

  int Count = 0;
  for (auto Element : SyntaxChildrenWithTokens(Root)) {
    ++Count;
  }
  EXPECT_EQ(0, Count);
}

TEST(SyntaxChildrenWithTokensTest, TokensOnlyChildren) {
  const auto Root = SyntaxNode::createRoot(NodeWithTokens);

  int TokenCount = 0;
  for (auto Element : SyntaxChildrenWithTokens(Root)) {
    EXPECT_TRUE(std::holds_alternative<SyntaxToken>(Element));
    ++TokenCount;
  }
  EXPECT_EQ(2, TokenCount);
}

TEST(SyntaxChildrenWithTokensTest, NodesOnlyChildren) {
  const auto Root = SyntaxNode::createRoot(NodeWithNodes);

  auto Children = SyntaxChildrenWithTokens(Root);
  auto It = Children.begin();

  const auto Element = *It++;
  EXPECT_TRUE(std::holds_alternative<SyntaxNode>(Element));
  EXPECT_EQ(std::get<SyntaxNode>(Element).getGreen(), NodeEmpty);
  EXPECT_EQ(It, Children.end());
}

TEST(SyntaxChildrenWithTokensTest, NodesAndTokensChildren) {
  const auto Root = SyntaxNode::createRoot(NodeMixed);

  auto Children = SyntaxChildrenWithTokens(Root);
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
  const auto ManyChildren = GreenNode(
      {.Value = 0},
      {
          GreenElement(GreenToken({.Value = 1}, U"(")),
          GreenElement(GreenNode(
              {.Value = 4},
              {GreenElement(GreenToken({.Value = 3}, U"*"))})),
          GreenElement(GreenNode(
              {.Value = 5},
              {GreenElement(GreenToken({.Value = 2}, U"4"))})),
          GreenElement(GreenNode(
              {.Value = 5},
              {GreenElement(GreenToken({.Value = 2}, U"3"))})),
          GreenElement(GreenToken({.Value = 1}, U")")),
      });

  const auto Root = SyntaxNode::createRoot(ManyChildren);
  auto Children = SyntaxChildrenWithTokens(Root);
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
  const auto Root = SyntaxNode::createRoot(NodeMixed);
  auto Children = SyntaxChildrenWithTokens(Root);

  EXPECT_EQ(Children.begin(), Children.begin());
}

TEST(SyntaxChildrenWithTokensTest, NotEquals) {
  const auto Root = SyntaxNode::createRoot(NodeMixed);
  auto Children = SyntaxChildrenWithTokens(Root);
  auto It = Children.begin();

  EXPECT_NE(++It, Children.begin());
}

} // namespace
