#include "yuzu/Syntax/Api.h"

#include "yuzu/Syntax/Green/Green.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <iterator>
#include <type_traits>
#include <vector>

namespace {
using yuzu::syntax::GreenElement;
using yuzu::syntax::GreenNode;
using yuzu::syntax::GreenToken;
using yuzu::syntax::api::SyntaxIterator;
using yuzu::syntax::api::SyntaxNode;

/// Local test enum standing in for a frontend-supplied SyntaxKind. Values
/// are arbitrary; only the underlying-uint16 round trip matters.
enum class TestKind : uint16_t {
  Root = 1,
  Inner = 2,
  Plus = 3,
  Ident = 4,
};

TEST(ApiSyntaxNodeTest, GetKindReturnsTypedEnum) {
  const auto green = GreenNode::create(static_cast<uint16_t>(TestKind::Root),
                                       std::vector<GreenElement>());
  const auto node = SyntaxNode<TestKind>::createRoot(green);

  static_assert(std::is_same_v<decltype(node.getKind()), TestKind>,
                "getKind() must return the wrapper's Kind, not the raw u16");
  EXPECT_EQ(node.getKind(), TestKind::Root);
}

TEST(ApiSyntaxNodeTest, CreateRootSetsOffsetAndIndexToZero) {
  const auto green = GreenNode::create(static_cast<uint16_t>(TestKind::Root),
                                       std::vector<GreenElement>());
  const auto node = SyntaxNode<TestKind>::createRoot(green);

  EXPECT_EQ(node.getOffset(), 0u);
  EXPECT_EQ(node.getIndex(), 0u);
  EXPECT_EQ(node.getParent(), nullptr);
}

TEST(ApiSyntaxNodeTest, GetGreenReturnsBackingNode) {
  const auto green = GreenNode::create(static_cast<uint16_t>(TestKind::Root),
                                       std::vector<GreenElement>());
  const auto node = SyntaxNode<TestKind>::createRoot(green);

  EXPECT_EQ(node.getGreen(), green);
}

TEST(ApiSyntaxNodeTest, GetFirstChildReturnsTypedNode) {
  const auto inner = GreenNode::create(static_cast<uint16_t>(TestKind::Inner),
                                       std::vector<GreenElement>());
  const auto root =
      GreenNode::create(static_cast<uint16_t>(TestKind::Root),
                        std::vector<GreenElement>{inner});
  const auto node = SyntaxNode<TestKind>::createRoot(root);

  const auto first = node.getFirstChild();
  ASSERT_TRUE(first.has_value());
  EXPECT_EQ(first->getKind(), TestKind::Inner);
}

TEST(ApiSyntaxNodeTest, GetFirstChildOnLeafReturnsNullopt) {
  const auto root = GreenNode::create(static_cast<uint16_t>(TestKind::Root),
                                      std::vector<GreenElement>());
  const auto node = SyntaxNode<TestKind>::createRoot(root);

  EXPECT_FALSE(node.getFirstChild().has_value());
  EXPECT_FALSE(node.getFirstChildOrToken().has_value());
}

TEST(ApiSyntaxNodeTest, EqualNodesCompareEqual) {
  const auto green = GreenNode::create(static_cast<uint16_t>(TestKind::Root),
                                       std::vector<GreenElement>());
  const auto a = SyntaxNode<TestKind>::createRoot(green);
  const auto b = SyntaxNode<TestKind>::createRoot(green);

  EXPECT_EQ(a, b);
}

TEST(ApiSyntaxNodeTest, DifferentKindNodesCompareUnequal) {
  const auto rootGreen = GreenNode::create(
      static_cast<uint16_t>(TestKind::Root), std::vector<GreenElement>());
  const auto innerGreen = GreenNode::create(
      static_cast<uint16_t>(TestKind::Inner), std::vector<GreenElement>());
  const auto a = SyntaxNode<TestKind>::createRoot(rootGreen);
  const auto b = SyntaxNode<TestKind>::createRoot(innerGreen);

  EXPECT_NE(a, b);
}

TEST(ApiSyntaxTokenTest, GetKindReturnsTypedEnum) {
  const auto token = GreenToken(static_cast<uint16_t>(TestKind::Plus), U"+");
  const auto root =
      GreenNode::create(static_cast<uint16_t>(TestKind::Root),
                        std::vector<GreenElement>{token});
  const auto node = SyntaxNode<TestKind>::createRoot(root);

  const auto first = node.getFirstChildOrToken();
  ASSERT_TRUE(first.has_value());
  ASSERT_TRUE(first->isToken());
  const auto &tok = first->getToken();

  static_assert(std::is_same_v<decltype(tok.getKind()), TestKind>,
                "SyntaxToken::getKind() must return the wrapper's Kind");
  EXPECT_EQ(tok.getKind(), TestKind::Plus);
}

TEST(ApiSyntaxElementTest, IsNodeAndIsTokenDispatch) {
  const auto token = GreenToken(static_cast<uint16_t>(TestKind::Plus), U"+");
  const auto inner = GreenNode::create(static_cast<uint16_t>(TestKind::Inner),
                                       std::vector<GreenElement>());
  const auto root = GreenNode::create(
      static_cast<uint16_t>(TestKind::Root),
      std::vector<GreenElement>{token, inner});
  const auto node = SyntaxNode<TestKind>::createRoot(root);

  auto it = node.getChildrenWithTokens().begin();
  EXPECT_TRUE((*it).isToken());
  ++it;
  EXPECT_TRUE((*it).isNode());
}

TEST(ApiSyntaxElementTest, GetKindForwardsToActiveAlternative) {
  const auto token = GreenToken(static_cast<uint16_t>(TestKind::Plus), U"+");
  const auto inner = GreenNode::create(static_cast<uint16_t>(TestKind::Inner),
                                       std::vector<GreenElement>());
  const auto root = GreenNode::create(
      static_cast<uint16_t>(TestKind::Root),
      std::vector<GreenElement>{token, inner});
  const auto node = SyntaxNode<TestKind>::createRoot(root);

  std::vector<TestKind> kinds;
  for (auto child : node.getChildrenWithTokens()) {
    kinds.push_back(child.getKind());
  }
  EXPECT_EQ(kinds,
            (std::vector<TestKind>{TestKind::Plus, TestKind::Inner}));
}

TEST(ApiSyntaxChildrenTest, EmptyRangeBeginEqualsEnd) {
  const auto root = GreenNode::create(static_cast<uint16_t>(TestKind::Root),
                                      std::vector<GreenElement>());
  const auto node = SyntaxNode<TestKind>::createRoot(root);
  const auto children = node.getChildren();

  EXPECT_EQ(children.begin(), children.end());
}

TEST(ApiSyntaxChildrenTest, IteratorYieldsTypedNodesAndSkipsTokens) {
  const auto token = GreenToken(static_cast<uint16_t>(TestKind::Plus), U"+");
  const auto inner1 = GreenNode::create(static_cast<uint16_t>(TestKind::Inner),
                                        std::vector<GreenElement>());
  const auto inner2 = GreenNode::create(static_cast<uint16_t>(TestKind::Inner),
                                        std::vector<GreenElement>());
  const auto root = GreenNode::create(
      static_cast<uint16_t>(TestKind::Root),
      std::vector<GreenElement>{inner1, token, inner2});
  const auto node = SyntaxNode<TestKind>::createRoot(root);

  std::vector<TestKind> kinds;
  for (auto child : node.getChildren()) {
    kinds.push_back(child.getKind());
  }
  EXPECT_EQ(kinds,
            (std::vector<TestKind>{TestKind::Inner, TestKind::Inner}));
}

TEST(ApiSyntaxChildrenWithTokensTest, IteratorYieldsBothNodesAndTokens) {
  const auto token = GreenToken(static_cast<uint16_t>(TestKind::Plus), U"+");
  const auto inner = GreenNode::create(static_cast<uint16_t>(TestKind::Inner),
                                       std::vector<GreenElement>());
  const auto root = GreenNode::create(
      static_cast<uint16_t>(TestKind::Root),
      std::vector<GreenElement>{inner, token});
  const auto node = SyntaxNode<TestKind>::createRoot(root);

  int nodeCount = 0;
  int tokenCount = 0;
  for (auto child : node.getChildrenWithTokens()) {
    if (child.isNode()) {
      ++nodeCount;
    } else {
      ++tokenCount;
    }
  }
  EXPECT_EQ(nodeCount, 1);
  EXPECT_EQ(tokenCount, 1);
}

TEST(ApiSyntaxIteratorTest, IteratorTraitsAreInputForwardOnly) {
  using It = SyntaxIterator<TestKind>;
  static_assert(
      std::is_same_v<It::iterator_category, std::input_iterator_tag>,
      "SyntaxIterator must declare input-iterator semantics");
  static_assert(std::is_same_v<It::value_type, SyntaxNode<TestKind>>,
                "SyntaxIterator value_type must be the wrapper");
}
} // namespace
