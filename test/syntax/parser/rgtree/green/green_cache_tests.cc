#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "syntax/parser/rgtree/green/green_cache.h"
#include "syntax/parser/rgtree/green/green_element.h"
#include "syntax/syntax_kind.h"

namespace {
using orion::syntax::CachedGreenElement;
using orion::syntax::GreenCache;
using orion::syntax::SyntaxKind;

constexpr size_t kMaxCachedNodeSize = 3;

constexpr auto kTestSyntaxKindPlus = SyntaxKind::kPlus;
constexpr auto kTestSyntaxKindMinus = SyntaxKind::kMinus;

const std::u32string kTestSource1 = U"hello world";
const std::u32string kTestSource2 = U"goodbye world";

TEST(GreenCacheTest, GetToken) {
  auto cache = GreenCache(kMaxCachedNodeSize);
  const auto entry = cache.GetToken(kTestSyntaxKindPlus, kTestSource1);

  // One in the cache, one held in this test method.
  EXPECT_EQ(2, entry.Element().UseCount());

  // Only one instance of this token.
  EXPECT_EQ(1, cache.TokenSize());
}

TEST(GreenCacheTest, GetTokensDifferentKind) {
  auto cache = GreenCache(kMaxCachedNodeSize);
  const auto entry1 = cache.GetToken(kTestSyntaxKindPlus, kTestSource1);
  const auto entry2 = cache.GetToken(kTestSyntaxKindMinus, kTestSource1);

  // One in the cache, one held in this test method.
  EXPECT_EQ(2, entry1.Element().UseCount());
  EXPECT_EQ(2, entry2.Element().UseCount());

  // Hashes for two distinct tokens should never be equal.
  EXPECT_NE(entry1.Hash(), entry2.Hash());

  // Two different tokens for entry1.Element(), and token 2.
  EXPECT_EQ(2, cache.TokenSize());
}

TEST(GreenCacheTest, GetTokensDifferentSource) {
  auto cache = GreenCache(kMaxCachedNodeSize);
  const auto entry1 = cache.GetToken(kTestSyntaxKindPlus, kTestSource1);
  const auto entry2 = cache.GetToken(kTestSyntaxKindPlus, kTestSource2);

  // One in the cache, one held in this test method.
  EXPECT_EQ(2, entry1.Element().UseCount());
  EXPECT_EQ(2, entry2.Element().UseCount());

  // Two different tokens for entry1.Element(), and entry2.Element().
  EXPECT_EQ(2, cache.TokenSize());
}

TEST(GreenCacheTest, GetNode) {
  auto cache = GreenCache(kMaxCachedNodeSize);

  const CachedGreenElement entry1 =
      cache.GetToken(kTestSyntaxKindPlus, kTestSource1);

  const CachedGreenElement entry2 =
      cache.GetToken(kTestSyntaxKindMinus, kTestSource2);

  auto children = std::vector{entry1, entry2};

  auto entry = cache.GetNode(SyntaxKind::kError, &children, 0);

  // The node should have two children.
  EXPECT_EQ(2, entry.Element().TryGetNode()->Children().size());

  // Children vector should have its elements removed.
  EXPECT_EQ(0, children.size());

  // One in the cache, one held in this test method.
  EXPECT_EQ(2, entry.Element().UseCount());

  // Two different tokens for entry1.Element(), and entry2.Element(). One node
  // for node.
  EXPECT_EQ(2, cache.TokenSize());
  EXPECT_EQ(1, cache.NodeSize());
}

TEST(GreenCacheTest, GetNodeLeftoverChildren) {
  auto cache = GreenCache(kMaxCachedNodeSize);

  const CachedGreenElement entry1 =
      cache.GetToken(kTestSyntaxKindPlus, kTestSource1);

  const CachedGreenElement entry2 =
      cache.GetToken(kTestSyntaxKindMinus, kTestSource2);

  auto children = std::vector{entry1, entry2};
  const auto entry = cache.GetNode(SyntaxKind::kError, &children, 1);

  // The node should have two children.
  EXPECT_EQ(1, entry.Element().TryGetNode()->Children().size());

  // Children vector should have its elements reduced.
  EXPECT_EQ(1, children.size());

  // One in the cache, one held in this test method.
  EXPECT_EQ(2, entry.Element().UseCount());

  // Two different tokens for entry1.Element(), and entry2.Element(). One node
  // for node.
  EXPECT_EQ(2, cache.TokenSize());
  EXPECT_EQ(1, cache.NodeSize());
}

TEST(GreenCacheTest, GetNodeDuplicateNodes) {
  auto cache = GreenCache(kMaxCachedNodeSize);

  const CachedGreenElement child1 =
      cache.GetToken(kTestSyntaxKindPlus, kTestSource1);

  const CachedGreenElement child2 =
      cache.GetToken(kTestSyntaxKindPlus, kTestSource1);

  auto children = std::vector{child1, child2};

  const auto entry1 = cache.GetNode(SyntaxKind::kError, &children, 1);
  const auto entry2 = cache.GetNode(SyntaxKind::kError, &children, 0);

  // Children vector should have its elements removed.
  EXPECT_EQ(0, children.size());

  // One token for entry1.Element() and entry2.Element(). One node for
  // entry1.Element() and entry2.Element().
  EXPECT_EQ(1, cache.TokenSize());
  EXPECT_EQ(1, cache.NodeSize());

  // Hashes for the same node should be the same.
  EXPECT_EQ(entry1.Hash(), entry2.Hash());

  // The node should have two children.
  EXPECT_EQ(1, entry1.Element().TryGetNode()->Children().size());
  EXPECT_EQ(1, entry2.Element().TryGetNode()->Children().size());

  // One in the cache, two held in this test method since the ndoes are the
  // same.
  EXPECT_EQ(3, entry1.Element().UseCount());
  EXPECT_EQ(3, entry2.Element().UseCount());
}

TEST(GreenCacheTest, GetNodeDuplicateNodesOverMaxCacheSize) {
  auto cache = GreenCache(0);

  const CachedGreenElement child1 =
      cache.GetToken(kTestSyntaxKindPlus, kTestSource1);

  const CachedGreenElement child2 =
      cache.GetToken(kTestSyntaxKindPlus, kTestSource1);

  auto children = std::vector{child1, child2};

  const auto entry1 = cache.GetNode(SyntaxKind::kError, &children, 1);
  const auto entry2 = cache.GetNode(SyntaxKind::kError, &children, 0);

  // Children vector should have its elements removed.
  EXPECT_EQ(0, children.size());

  // One token for entry1.Element() and entry2.Element(), however no nodes
  // should be cached.
  EXPECT_EQ(1, cache.TokenSize());
  EXPECT_EQ(0, cache.NodeSize());

  // Hashes for the same node should be the same.
  EXPECT_EQ(0, entry1.Hash());
  EXPECT_EQ(0, entry2.Hash());

  // The node should have two children.
  EXPECT_EQ(1, entry1.Element().TryGetNode()->Children().size());
  EXPECT_EQ(1, entry2.Element().TryGetNode()->Children().size());

  // At this point, each node is *not* cached.
  EXPECT_EQ(1, entry1.Element().UseCount());
  EXPECT_EQ(1, entry2.Element().UseCount());
}
}  // namespace
