#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "syntax/parser/rgtree/green/green_cache.h"
#include "syntax/parser/rgtree/green/green_element.h"
#include "syntax/parser/rgtree/green/green_node.h"
#include "syntax/parser/rgtree/green/green_token.h"
#include "syntax/parser/syntax_kind.h"

namespace {
constexpr size_t kMaxCachedNodeSize = 3;

constexpr orion::syntax::SyntaxKind kTestSyntaxKind1 =
    orion::syntax::SyntaxKind::kPlus;

constexpr orion::syntax::SyntaxKind kTestSyntaxKind2 =
    orion::syntax::SyntaxKind::kMinus;

const std::u32string kTestSource1 = U"hello world";
const std::u32string kTestSource2 = U"goodbye world";

TEST(GreenCacheTest, GetToken) {
  auto cache = orion::syntax::GreenCache(kMaxCachedNodeSize);
  auto [hash, token] = cache.GetToken(kTestSyntaxKind1, kTestSource1);

  // One in the cache, one held in this test method.
  EXPECT_EQ(2, token.UseCount());

  // Only one instance of this token.
  EXPECT_EQ(1, cache.TokenSize());
}

TEST(GreenCacheTest, GetTokensDifferentKind) {
  auto cache = orion::syntax::GreenCache(kMaxCachedNodeSize);
  auto [hash1, token1] = cache.GetToken(kTestSyntaxKind1, kTestSource1);
  auto [hash2, token2] = cache.GetToken(kTestSyntaxKind2, kTestSource1);

  // One in the cache, one held in this test method.
  EXPECT_EQ(2, token1.UseCount());
  EXPECT_EQ(2, token2.UseCount());

  // Hashes for two distinct tokens should never be equal.
  EXPECT_NE(hash1, hash2);

  // Two different tokens for token1, and token 2.
  EXPECT_EQ(2, cache.TokenSize());
}

TEST(GreenCacheTest, GetTokensDifferentSource) {
  auto cache = orion::syntax::GreenCache(kMaxCachedNodeSize);
  auto [hash1, token1] = cache.GetToken(kTestSyntaxKind1, kTestSource1);
  auto [hash2, token2] = cache.GetToken(kTestSyntaxKind1, kTestSource2);

  // One in the cache, one held in this test method.
  EXPECT_EQ(2, token1.UseCount());
  EXPECT_EQ(2, token2.UseCount());

  // Two different tokens for token1, and token2.
  EXPECT_EQ(2, cache.TokenSize());
}

TEST(GreenCacheTest, GetNode) {
  auto cache = orion::syntax::GreenCache(kMaxCachedNodeSize);

  const orion::syntax::CachedGreenElement entry1 =
      cache.GetToken(kTestSyntaxKind1, kTestSource1);

  const orion::syntax::CachedGreenElement entry2 =
      cache.GetToken(kTestSyntaxKind2, kTestSource2);

  auto children = std::vector{entry1, entry2};

  auto [hash, node] =
      cache.GetNode(orion::syntax::SyntaxKind::kError, children, 0);

  // The node should have two children.
  EXPECT_EQ(2, node.TryGetNode()->Children().size());

  // Children vector should have its elements removed.
  EXPECT_EQ(0, children.size());

  // One in the cache, one held in this test method.
  EXPECT_EQ(2, node.UseCount());

  // Two different tokens for token1, and token2. One node for node.
  EXPECT_EQ(2, cache.TokenSize());
  EXPECT_EQ(1, cache.NodeSize());
}

TEST(GreenCacheTest, GetNodeLeftoverChildren) {
  auto cache = orion::syntax::GreenCache(kMaxCachedNodeSize);

  const orion::syntax::CachedGreenElement entry1 =
      cache.GetToken(kTestSyntaxKind1, kTestSource1);

  const orion::syntax::CachedGreenElement entry2 =
      cache.GetToken(kTestSyntaxKind2, kTestSource2);

  auto children = std::vector{entry1, entry2};

  auto [hash, node] =
      cache.GetNode(orion::syntax::SyntaxKind::kError, children, 1);

  // The node should have two children.
  EXPECT_EQ(1, node.TryGetNode()->Children().size());

  // Children vector should have its elements reduced.
  EXPECT_EQ(1, children.size());

  // One in the cache, one held in this test method.
  EXPECT_EQ(2, node.UseCount());

  // Two different tokens for token1, and token2. One node for node.
  EXPECT_EQ(2, cache.TokenSize());
  EXPECT_EQ(1, cache.NodeSize());
}

TEST(GreenCacheTest, GetNodeDuplicateNodes) {
  auto cache = orion::syntax::GreenCache(kMaxCachedNodeSize);

  const orion::syntax::CachedGreenElement entry1 =
      cache.GetToken(kTestSyntaxKind1, kTestSource1);

  const orion::syntax::CachedGreenElement entry2 =
      cache.GetToken(kTestSyntaxKind1, kTestSource1);

  auto children = std::vector{entry1, entry2};

  auto [hash1, node1] =
      cache.GetNode(orion::syntax::SyntaxKind::kError, children, 1);

  auto [hash2, node2] =
      cache.GetNode(orion::syntax::SyntaxKind::kError, children, 0);

  // Hashes for the same node should be the same.
  EXPECT_EQ(hash1, hash2);

  // The node should have two children.
  EXPECT_EQ(1, node1.TryGetNode()->Children().size());
  EXPECT_EQ(1, node2.TryGetNode()->Children().size());

  // Children vector should have its elements removed.
  EXPECT_EQ(0, children.size());

  // One in the cache, two held in this test method since the ndoes are the
  // same.
  EXPECT_EQ(3, node1.UseCount());
  EXPECT_EQ(3, node2.UseCount());

  // One token for token1 and token2. One node for node1 and node2.
  EXPECT_EQ(1, cache.TokenSize());
  EXPECT_EQ(1, cache.NodeSize());
}

TEST(GreenCacheTest, GetNodeDuplicateNodesOverMaxCacheSize) {
  auto cache = orion::syntax::GreenCache(0);

  const orion::syntax::CachedGreenElement entry1 =
      cache.GetToken(kTestSyntaxKind1, kTestSource1);

  const orion::syntax::CachedGreenElement entry2 =
      cache.GetToken(kTestSyntaxKind1, kTestSource1);

  auto children = std::vector{entry1, entry2};

  auto [hash1, node1] =
      cache.GetNode(orion::syntax::SyntaxKind::kError, children, 1);

  auto [hash2, node2] =
      cache.GetNode(orion::syntax::SyntaxKind::kError, children, 0);

  // Hashes for the same node should be the same.
  EXPECT_EQ(0, hash1);
  EXPECT_EQ(0, hash2);

  // The node should have two children.
  EXPECT_EQ(1, node1.TryGetNode()->Children().size());
  EXPECT_EQ(1, node2.TryGetNode()->Children().size());

  // Children vector should have its elements removed.
  EXPECT_EQ(0, children.size());

  // At this point, each node is *not* cached.
  EXPECT_EQ(1, node1.UseCount());
  EXPECT_EQ(1, node2.UseCount());

  // One token for token1 and token2, however no nodes should be cached.
  EXPECT_EQ(1, cache.TokenSize());
  EXPECT_EQ(0, cache.NodeSize());
}
}  // namespace