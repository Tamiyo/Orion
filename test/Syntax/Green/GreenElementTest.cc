#include "Syntax/Green/Green.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace {
using yuzu::syntax::GreenElement;
using yuzu::syntax::GreenNode;
using yuzu::syntax::GreenToken;

const auto Node = GreenNode::create(12, std::vector<GreenElement>{});
const auto NodeElement = GreenElement(Node);
const auto Token = GreenToken(2, U"3");
const auto TokenElement = GreenElement(Token);

TEST(GreenElementTest, GetNode) { EXPECT_EQ(Node, NodeElement.getNode()); }

TEST(GreenElementTest, GetIfNode) {
  EXPECT_EQ(Node, *NodeElement.getIfNode());
  EXPECT_EQ(nullptr, NodeElement.getIfToken());
}

TEST(GreenElementTest, GetToken) { EXPECT_EQ(Token, TokenElement.getToken()); }

TEST(GreenElementTest, GetIfToken) {
  EXPECT_EQ(Token, *TokenElement.getIfToken());
  EXPECT_EQ(nullptr, TokenElement.getIfNode());
}

TEST(GreenElementTest, IsNode) {
  EXPECT_TRUE(NodeElement.isNode());
  EXPECT_FALSE(NodeElement.isToken());
}

TEST(GreenElementTest, IsToken) {
  EXPECT_TRUE(TokenElement.isToken());
  EXPECT_FALSE(TokenElement.isNode());
}

TEST(GreenElementTest, GetKind) {
  // Node has a kind of 12, Token has a kind of 2.
  EXPECT_EQ(Node.getKind(), NodeElement.getKind());
  EXPECT_EQ(Token.getKind(), TokenElement.getKind());
}

TEST(GreenElementTest, GetWidth) {
  // Node has a width of 0, Token has a width of 1.
  EXPECT_EQ(Node.getWidth(), NodeElement.getWidth());
  EXPECT_EQ(Token.getWidth(), TokenElement.getWidth());
}

TEST(GreenElementTest, GetUseCount) {
  // Node and Token have 1 usage held by the GreenElement, and 1 usage held by
  // themselves. In a normal scenario, ownership of the Node and Token would be
  // moved into the GreenElement, having a useCount of 1.
  EXPECT_EQ(Node.getUseCount(), NodeElement.getUseCount());
  EXPECT_EQ(Token.getUseCount(), TokenElement.getUseCount());
}

TEST(GreenElementTest, Equals) {
  EXPECT_TRUE(GreenElement(Node) == NodeElement);
  EXPECT_TRUE(GreenElement(Token) == TokenElement);
}

TEST(GreenElementTest, NotEquals) {
  EXPECT_FALSE(GreenElement(Token) == NodeElement);
  EXPECT_FALSE(GreenElement(Node) == TokenElement);
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
