#include "syntax/green/green_cache.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "syntax/green/green_node.h"
#include "syntax/green/green_token.h"

namespace {
using yuzu::syntax::GreenCache;
using yuzu::syntax::SyntaxKind;

constexpr size_t kMaxCachedNodeSize = 3;
constexpr SyntaxKind kTestSyntaxKindZero = SyntaxKind{.value = 0};
constexpr SyntaxKind kTestSyntaxKindOne = SyntaxKind{.value = 1};
constexpr SyntaxKind kTestSyntaxKindThree = SyntaxKind{.value = 2};

const std::u32string kTestSource1 = U"hello world";
const std::u32string kTestSource2 = U"goodbye world";

TEST(GreenCacheTest, GetToken) {
  auto cache = GreenCache(kMaxCachedNodeSize);
  const auto entry = cache.GetToken(kTestSyntaxKindZero, kTestSource1);

  // One in the cache, one held in this test method.
  EXPECT_EQ(2, entry.element.UseCount());

  // Only one instance of this token.
  EXPECT_EQ(1, cache.TokenSize());
}

TEST(GreenCacheTest, GetTokensDifferentKind) {
  auto cache = GreenCache(kMaxCachedNodeSize);
  const auto entry1 = cache.GetToken(kTestSyntaxKindZero, kTestSource1);
  const auto entry2 = cache.GetToken(kTestSyntaxKindOne, kTestSource1);

  // One in the cache, one held in this test method.
  EXPECT_EQ(2, entry1.element.UseCount());
  EXPECT_EQ(2, entry2.element.UseCount());

  // Hashes for two distinct tokens should never be equal.
  EXPECT_NE(entry1.hash, entry2.hash);

  // Two different tokens for entry1.element, and token 2.
  EXPECT_EQ(2, cache.TokenSize());
}

TEST(GreenCacheTest, GetTokensDifferentSource) {
  auto cache = GreenCache(kMaxCachedNodeSize);
  const auto entry1 = cache.GetToken(kTestSyntaxKindZero, kTestSource1);
  const auto entry2 = cache.GetToken(kTestSyntaxKindZero, kTestSource2);

  // One in the cache, one held in this test method.
  EXPECT_EQ(2, entry1.element.UseCount());
  EXPECT_EQ(2, entry2.element.UseCount());

  // Two different tokens for entry1.element, and entry2.element.
  EXPECT_EQ(2, cache.TokenSize());
}

TEST(GreenCacheTest, GetNode) {
  auto cache = GreenCache(kMaxCachedNodeSize);

  const GreenCache::Entry entry1 =
      cache.GetToken(kTestSyntaxKindZero, kTestSource1);

  const GreenCache::Entry entry2 =
      cache.GetToken(kTestSyntaxKindOne, kTestSource2);

  auto children = std::vector{entry1, entry2};

  auto entry = cache.GetNode(kTestSyntaxKindThree, &children, 0);

  // The node should have two children.
  EXPECT_EQ(2, entry.element.TryGetNode()->Children().size());

  // Children vector should have its elements removed.
  EXPECT_EQ(0, children.size());

  // One in the cache, one held in this test method.
  EXPECT_EQ(2, entry.element.UseCount());

  // Two different tokens for entry1.element, and entry2.element. One node
  // for node.
  EXPECT_EQ(2, cache.TokenSize());
  EXPECT_EQ(1, cache.NodeSize());
}

TEST(GreenCacheTest, GetNodeLeftoverChildren) {
  auto cache = GreenCache(kMaxCachedNodeSize);

  const GreenCache::Entry entry1 =
      cache.GetToken(kTestSyntaxKindZero, kTestSource1);

  const GreenCache::Entry entry2 =
      cache.GetToken(kTestSyntaxKindOne, kTestSource2);

  auto children = std::vector{entry1, entry2};
  const auto entry = cache.GetNode(kTestSyntaxKindThree, &children, 1);

  // The node should have two children.
  EXPECT_EQ(1, entry.element.TryGetNode()->Children().size());

  // Children vector should have its elements reduced.
  EXPECT_EQ(1, children.size());

  // One in the cache, one held in this test method.
  EXPECT_EQ(2, entry.element.UseCount());

  // Two different tokens for entry1.element, and entry2.element. One node
  // for node.
  EXPECT_EQ(2, cache.TokenSize());
  EXPECT_EQ(1, cache.NodeSize());
}

TEST(GreenCacheTest, GetNodeDuplicateNodes) {
  auto cache = GreenCache(kMaxCachedNodeSize);

  const GreenCache::Entry child1 =
      cache.GetToken(kTestSyntaxKindZero, kTestSource1);

  const GreenCache::Entry child2 =
      cache.GetToken(kTestSyntaxKindZero, kTestSource1);

  auto children = std::vector{child1, child2};

  const auto entry1 = cache.GetNode(kTestSyntaxKindThree, &children, 1);
  const auto entry2 = cache.GetNode(kTestSyntaxKindThree, &children, 0);

  // Children vector should have its elements removed.
  EXPECT_EQ(0, children.size());

  // One token for entry1.element and entry2.element. One node for
  // entry1.element and entry2.element.
  EXPECT_EQ(1, cache.TokenSize());
  EXPECT_EQ(1, cache.NodeSize());

  // Hashes for the same node should be the same.
  EXPECT_EQ(entry1.hash, entry2.hash);

  // The node should have two children.
  EXPECT_EQ(1, entry1.element.TryGetNode()->Children().size());
  EXPECT_EQ(1, entry2.element.TryGetNode()->Children().size());

  // One in the cache, two held in this test method since the ndoes are the
  // same.
  EXPECT_EQ(3, entry1.element.UseCount());
  EXPECT_EQ(3, entry2.element.UseCount());
}

TEST(GreenCacheTest, GetNodeDuplicateNodesOverMaxCacheSize) {
  auto cache = GreenCache(0);

  const GreenCache::Entry child1 =
      cache.GetToken(kTestSyntaxKindZero, kTestSource1);

  const GreenCache::Entry child2 =
      cache.GetToken(kTestSyntaxKindZero, kTestSource1);

  auto children = std::vector{child1, child2};

  const auto entry1 = cache.GetNode(kTestSyntaxKindThree, &children, 1);
  const auto entry2 = cache.GetNode(kTestSyntaxKindThree, &children, 0);

  // Children vector should have its elements removed.
  EXPECT_EQ(0, children.size());

  // One token for entry1.element and entry2.element, however no nodes
  // should be cached.
  EXPECT_EQ(1, cache.TokenSize());
  EXPECT_EQ(0, cache.NodeSize());

  // Hashes for the same node should be the same.
  EXPECT_EQ(0, entry1.hash);
  EXPECT_EQ(0, entry2.hash);

  // The node should have two children.
  EXPECT_EQ(1, entry1.element.TryGetNode()->Children().size());
  EXPECT_EQ(1, entry2.element.TryGetNode()->Children().size());

  // At this point, each node is *not* cached.
  EXPECT_EQ(1, entry1.element.UseCount());
  EXPECT_EQ(1, entry2.element.UseCount());
}
}  // namespace
