#include "yuzu/Syntax/Green/GreenCache.h"

#include "yuzu/Syntax/Green/Green.h"
#include "yuzu/Syntax/Green/GreenIterator.h"
#include "yuzu/lib/Syntax/Green/GreenCache.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <string>
#include <vector>

namespace {
using yuzu::syntax::GreenCache;
using yuzu::syntax::GreenCacheEntry;
using yuzu::syntax::SyntaxKind;

constexpr size_t maxCachedNodeSize = 3;
constexpr SyntaxKind syntaxKindZero = 0;
constexpr SyntaxKind syntaxKindOne = 1;
constexpr SyntaxKind syntaxKindTwo = 2;

const std::u32string source1 = U"hello world";
const std::u32string source2 = U"goodbye world";

TEST(GreenCacheTest, GetToken) {
  auto cache = GreenCache(maxCachedNodeSize);
  const auto entry = cache.getToken(syntaxKindZero, source1);

  // One in the cache, one held in this test method.
  EXPECT_EQ(2, entry.element.getUseCount());

  // Only one instance of this token.
  EXPECT_EQ(1, cache.getTokenSize());
}

TEST(GreenCacheTest, GetTokensDifferentKind) {
  auto cache = GreenCache(maxCachedNodeSize);
  const auto entry1 = cache.getToken(syntaxKindZero, source1);
  const auto entry2 = cache.getToken(syntaxKindOne, source1);

  // One in the cache, one held in this test method.
  EXPECT_EQ(2, entry1.element.getUseCount());
  EXPECT_EQ(2, entry1.element.getUseCount());

  // Hashes for two distinct tokens should never be equal.
  EXPECT_NE(entry1.hash, entry2.hash);

  // Two different tokens for entry1.Element, and token 2.
  EXPECT_EQ(2, cache.getTokenSize());
}

TEST(GreenCacheTest, GetTokensDifferentSource) {
  auto cache = GreenCache(maxCachedNodeSize);
  const auto entry1 = cache.getToken(syntaxKindZero, source1);
  const auto entry2 = cache.getToken(syntaxKindZero, source2);

  // One in the cache, one held in this test method.
  EXPECT_EQ(2, entry1.element.getUseCount());
  EXPECT_EQ(2, entry2.element.getUseCount());

  // Two different tokens for entry1.Element, and entry2.Element.
  EXPECT_EQ(2, cache.getTokenSize());
}

TEST(GreenCacheTest, GetNode) {
  auto cache = GreenCache(maxCachedNodeSize);

  const GreenCacheEntry entry1 = cache.getToken(syntaxKindZero, source1);

  const GreenCacheEntry entry2 = cache.getToken(syntaxKindOne, source2);

  auto children = std::vector{entry1, entry2};

  auto entry = cache.getNode(syntaxKindTwo, &children, 0);

  // The node should have two children.
  EXPECT_EQ(2, entry.element.getNode().getChildren().size());

  // children vector should have its elements removed.
  EXPECT_EQ(0, children.size());

  // One in the cache, one held in this test method.
  EXPECT_EQ(2, entry.element.getUseCount());

  // Two different tokens for entry1.Element, and entry2.Element. One node
  // for node.
  EXPECT_EQ(2, cache.getTokenSize());
  EXPECT_EQ(1, cache.getNodeSize());
}

TEST(GreenCacheTest, GetNodeLeftoverChildren) {
  auto cache = GreenCache(maxCachedNodeSize);

  const GreenCacheEntry entry1 = cache.getToken(syntaxKindZero, source1);

  const GreenCacheEntry entry2 = cache.getToken(syntaxKindOne, source2);

  auto children = std::vector{entry1, entry2};
  const auto entry = cache.getNode(syntaxKindTwo, &children, 1);

  // The node should have two children.
  EXPECT_EQ(1, entry.element.getNode().getChildren().size());

  // children vector should have its elements reduced.
  EXPECT_EQ(1, children.size());

  // One in the cache, one held in this test method.
  EXPECT_EQ(2, entry.element.getUseCount());

  // Two different tokens for entry1.Element, and entry2.Element. One node
  // for node.
  EXPECT_EQ(2, cache.getTokenSize());
  EXPECT_EQ(1, cache.getNodeSize());
}

TEST(GreenCacheTest, GetNodeDuplicateNodes) {
  auto cache = GreenCache(maxCachedNodeSize);

  const GreenCacheEntry child1 = cache.getToken(syntaxKindZero, source1);

  const GreenCacheEntry child2 = cache.getToken(syntaxKindZero, source1);

  auto children = std::vector{child1, child2};

  const auto entry1 = cache.getNode(syntaxKindTwo, &children, 1);
  const auto entry2 = cache.getNode(syntaxKindTwo, &children, 0);

  // children vector should have its elements removed.
  EXPECT_EQ(0, children.size());

  // One token for entry1.Element and entry2.Element. One node for
  // entry1.Element and entry2.Element.
  EXPECT_EQ(1, cache.getTokenSize());
  EXPECT_EQ(1, cache.getNodeSize());

  // Hashes for the same node should be the same.
  EXPECT_EQ(entry1.hash, entry2.hash);

  // The node should have two children.
  EXPECT_EQ(1, entry1.element.getNode().getChildren().size());
  EXPECT_EQ(1, entry1.element.getNode().getChildren().size());

  // One in the cache, two held in this test method since the ndoes are the
  // same.
  EXPECT_EQ(3, entry1.element.getUseCount());
  EXPECT_EQ(3, entry2.element.getUseCount());
}

TEST(GreenCacheTest, GetNodeDuplicateNodesOverMaxCacheSize) {
  auto cache = GreenCache(0);

  const GreenCacheEntry child1 = cache.getToken(syntaxKindZero, source1);

  const GreenCacheEntry child2 = cache.getToken(syntaxKindZero, source1);

  auto children = std::vector{child1, child2};

  const auto entry1 = cache.getNode(syntaxKindTwo, &children, 1);
  const auto entry2 = cache.getNode(syntaxKindTwo, &children, 0);

  // children vector should have its elements removed.
  EXPECT_EQ(0, children.size());

  // One token for entry1.Element and entry2.Element, however no nodes
  // should be cached.
  EXPECT_EQ(1, cache.getTokenSize());
  EXPECT_EQ(0, cache.getNodeSize());

  // Hashes for the same node should be the same.
  EXPECT_EQ(0, entry1.hash);
  EXPECT_EQ(0, entry2.hash);

  // The node should have two children.
  EXPECT_EQ(1, entry1.element.getNode().getChildren().size());
  EXPECT_EQ(1, entry2.element.getNode().getChildren().size());

  // At this point, each node is *not* cached.
  EXPECT_EQ(1, entry1.element.getUseCount());
  EXPECT_EQ(1, entry2.element.getUseCount());
}
} // namespace
