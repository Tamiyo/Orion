#include "Syntax/SyntaxIterator.h"

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "Syntax/Green/Green.h"
#include "Syntax/Syntax.h"
#include "Syntax/SyntaxKind.h"

namespace {
using yuzu::syntax::GreenElement;
using yuzu::syntax::GreenNode;
using yuzu::syntax::GreenToken;
using yuzu::syntax::SyntaxKind;
using yuzu::syntax::SyntaxNode;
using yuzu::syntax::SyntaxNodeChildren;

const auto GreenToken1 = GreenToken(GreenToken({.Value = 1}, U"token"));

const auto GreenToken2 = GreenToken(GreenToken({.Value = 1}, U"token"));

const auto GreenNodeNoChildren = GreenNode(GreenNode({.Value = 10}, {}));

const auto GreenNodeTokensChildren = GreenNode(GreenNode(
    {.Value = 11}, {GreenElement(GreenToken1), GreenElement(GreenToken2)}));

const auto GreenNodeNodesChildren =
    GreenNode(GreenNode({.Value = 12}, {GreenElement(GreenNodeNoChildren)}));

const auto GreenNodeNodesAndTokensChildren = GreenNode(GreenNode(
    {.Value = 13},
    {GreenElement(GreenToken1), GreenElement(GreenNodeNoChildren),
     GreenElement(GreenToken2), GreenElement(GreenNodeTokensChildren)}));

TEST(SyntaxNodeIteratorTest, NoChildren) {
  const auto SyntaxNode = SyntaxNode::createRoot(GreenNodeNoChildren);

  SyntaxNodeChildren Children = SyntaxNodeChildren(SyntaxNode);
  int ChildrenCount = 0;
  for (auto It = Children.begin(); It != Children.end(); It++) {
    ChildrenCount += 1;
  }

  EXPECT_EQ(0, ChildrenCount);
}

TEST(SyntaxNodeIteratorTest, TokensOnlyChildren) {
  const auto SyntaxNode = SyntaxNode::createRoot(GreenNodeTokensChildren);

  SyntaxNodeChildren Children = SyntaxNodeChildren(SyntaxNode);
  int ChildrenCount = 0;
  for (auto It = Children.begin(), End = Children.end(); It != End; It++) {
    ChildrenCount += 1;
  }

  EXPECT_EQ(0, ChildrenCount);
}

TEST(SyntaxNodeIteratorTest, NodesOnlyChildren) {
  const auto SyntaxNode = SyntaxNode::createRoot(GreenNodeNodesChildren);

  SyntaxNodeChildren Children = SyntaxNodeChildren(SyntaxNode);
  SyntaxNodeChildren::Iterator It = Children.begin();

  EXPECT_EQ((*(It++)).getGreen(), GreenNodeNoChildren);
  EXPECT_EQ(It, Children.end());
}

TEST(SyntaxNodeIteratorTest, NodesAndTokensChildren) {
  const auto SyntaxNode =
      SyntaxNode::createRoot(GreenNodeNodesAndTokensChildren);

  SyntaxNodeChildren Children = SyntaxNodeChildren(SyntaxNode);
  SyntaxNodeChildren::Iterator It = Children.begin();

  EXPECT_EQ((*(It++)).getGreen(), GreenNodeNoChildren);
  EXPECT_EQ((*(It++)).getGreen(), GreenNodeTokensChildren);
  EXPECT_EQ(It, Children.end());
}

TEST(SyntaxNodeIteratorTest, Equals) {
  const auto SyntaxNode = SyntaxNode::createRoot(GreenNodeNodesChildren);
  SyntaxNodeChildren Children = SyntaxNodeChildren(SyntaxNode);

  EXPECT_EQ(Children.begin(), Children.begin());
}

TEST(SyntaxNodeIteratorTest, NotEquals) {
  const auto SyntaxNode = SyntaxNode::createRoot(GreenNodeNodesChildren);

  SyntaxNodeChildren Children = SyntaxNodeChildren(SyntaxNode);
  SyntaxNodeChildren::Iterator It = Children.begin();

  EXPECT_NE(++It, Children.begin());
}

TEST(SyntaxNodeIteratorTest, NodesWithOffsets) {
  const auto GreenNodeManyChildren = GreenNode(GreenNode(
      {.Value = 0},
      {
          GreenElement(GreenToken(GreenToken({.Value = 1}, U"("))),
          GreenElement(GreenNode(GreenNode(
              {.Value = 4},
              {GreenElement(GreenToken(GreenToken({.Value = 3}, U"*")))}))),
          GreenElement(GreenNode(GreenNode(
              {.Value = 5},
              {GreenElement(GreenToken(GreenToken({.Value = 2}, U"4")))}))),
          GreenElement(GreenNode(GreenNode(
              {.Value = 5},
              {GreenElement(GreenToken(GreenToken({.Value = 2}, U"3")))}))),
          GreenElement(GreenToken(GreenToken({.Value = 1}, U")"))),
      }));

  const auto SyntaxNode = SyntaxNode::createRoot(GreenNodeManyChildren);
  SyntaxNodeChildren Children = SyntaxNodeChildren(SyntaxNode);
  SyntaxNodeChildren::Iterator It = Children.begin();

  EXPECT_EQ(1, (*(It++)).getOffset());
  EXPECT_EQ(2, (*(It++)).getOffset());
  EXPECT_EQ(3, (*(It++)).getOffset());
  EXPECT_EQ(It, Children.end());
}

} // namespace
