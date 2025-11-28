#include "yuzu/Syntax/Green/Green.h"

#include <gmock/gmock.h>
#include "gtest/gtest.h"

namespace {
using yuzu::syntax::GreenElement;
using yuzu::syntax::GreenNode;
using yuzu::syntax::GreenToken;

const auto node = GreenNode::create(12, std::vector<GreenElement>{});
const auto nodeElement = GreenElement(node);
const auto token = GreenToken(2, U"3");
const auto tokenElement = GreenElement(token);

TEST(GreenElementTest, GetNode) { EXPECT_EQ(node, nodeElement.getNode()); }

TEST(GreenElementTest, GetIfNode) {
  EXPECT_EQ(node, *nodeElement.getIfNode());
  EXPECT_EQ(nullptr, nodeElement.getIfToken());
}

TEST(GreenElementTest, GetToken) { EXPECT_EQ(token, tokenElement.getToken()); }

TEST(GreenElementTest, GetIfToken) {
  EXPECT_EQ(token, *tokenElement.getIfToken());
  EXPECT_EQ(nullptr, tokenElement.getIfNode());
}

TEST(GreenElementTest, IsNode) {
  EXPECT_TRUE(nodeElement.isNode());
  EXPECT_FALSE(nodeElement.isToken());
}

TEST(GreenElementTest, IsToken) {
  EXPECT_TRUE(tokenElement.isToken());
  EXPECT_FALSE(tokenElement.isNode());
}

TEST(GreenElementTest, GetKind) {
  // node has a kind of 12, token has a kind of 2.
  EXPECT_EQ(node.getKind(), nodeElement.getKind());
  EXPECT_EQ(token.getKind(), tokenElement.getKind());
}

TEST(GreenElementTest, GetWidth) {
  // node has a width of 0, token has a width of 1.
  EXPECT_EQ(node.getWidth(), nodeElement.getWidth());
  EXPECT_EQ(token.getWidth(), tokenElement.getWidth());
}

TEST(GreenElementTest, GetUseCount) {
  // node and token have 1 usage held by the GreenElement, and 1 usage held by
  // themselves. In a normal scenario, ownership of the node and token would be
  // moved into the GreenElement, having a useCount of 1.
  EXPECT_EQ(node.getUseCount(), nodeElement.getUseCount());
  EXPECT_EQ(token.getUseCount(), tokenElement.getUseCount());
}

TEST(GreenElementTest, Equals) {
  EXPECT_TRUE(GreenElement(node) == nodeElement);
  EXPECT_TRUE(GreenElement(token) == tokenElement);
}

TEST(GreenElementTest, NotEquals) {
  EXPECT_FALSE(GreenElement(token) == nodeElement);
  EXPECT_FALSE(GreenElement(node) == tokenElement);
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
