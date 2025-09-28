#include "Syntax/Green/GreenCache.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Syntax/Green/Green.h"

namespace {
using yuzu::syntax::GreenCache;
using yuzu::syntax::SyntaxKind;

constexpr size_t kMaxCachedNodeSize = 3;
constexpr SyntaxKind kTestSyntaxKindZero = SyntaxKind{.Value = 0};
constexpr SyntaxKind kTestSyntaxKindOne = SyntaxKind{.Value = 1};
constexpr SyntaxKind kTestSyntaxKindThree = SyntaxKind{.Value = 2};

const std::u32string kTestSource1 = U"hello world";
const std::u32string kTestSource2 = U"goodbye world";

TEST(GreenCacheTest, GetToken) {
  auto Cache = GreenCache(kMaxCachedNodeSize);
  const auto Entry = Cache.getToken(kTestSyntaxKindZero, kTestSource1);

  // One in the cache, one held in this test method.
  EXPECT_EQ(2, Entry.Element.getUseCount());

  // Only one instance of this token.
  EXPECT_EQ(1, Cache.getTokenSize());
}

TEST(GreenCacheTest, GetTokensDifferentKind) {
  auto Cache = GreenCache(kMaxCachedNodeSize);
  const auto Entry1 = Cache.getToken(kTestSyntaxKindZero, kTestSource1);
  const auto Entry2 = Cache.getToken(kTestSyntaxKindOne, kTestSource1);

  // One in the cache, one held in this test method.
  EXPECT_EQ(2, Entry1.Element.getUseCount());
  EXPECT_EQ(2, Entry2.Element.getUseCount());

  // Hashes for two distinct tokens should never be equal.
  EXPECT_NE(Entry1.Hash, Entry2.Hash);

  // Two different tokens for Entry1.Element, and token 2.
  EXPECT_EQ(2, Cache.getTokenSize());
}

TEST(GreenCacheTest, GetTokensDifferentSource) {
  auto Cache = GreenCache(kMaxCachedNodeSize);
  const auto Entry1 = Cache.getToken(kTestSyntaxKindZero, kTestSource1);
  const auto Entry2 = Cache.getToken(kTestSyntaxKindZero, kTestSource2);

  // One in the cache, one held in this test method.
  EXPECT_EQ(2, Entry1.Element.getUseCount());
  EXPECT_EQ(2, Entry2.Element.getUseCount());

  // Two different tokens for Entry1.Element, and Entry2.Element.
  EXPECT_EQ(2, Cache.getTokenSize());
}

TEST(GreenCacheTest, GetNode) {
  auto Cache = GreenCache(kMaxCachedNodeSize);

  const GreenCache::Entry Entry1 =
      Cache.getToken(kTestSyntaxKindZero, kTestSource1);

  const GreenCache::Entry Entry2 =
      Cache.getToken(kTestSyntaxKindOne, kTestSource2);

  auto Children = std::vector{Entry1, Entry2};

  auto Entry = Cache.getNode(kTestSyntaxKindThree, &Children, 0);

  // The node should have two children.
  EXPECT_EQ(2, Entry.Element.tryGetNode()->getChildren().size());

  // Children vector should have its elements removed.
  EXPECT_EQ(0, Children.size());

  // One in the cache, one held in this test method.
  EXPECT_EQ(2, Entry.Element.getUseCount());

  // Two different tokens for Entry1.Element, and Entry2.Element. One node
  // for node.
  EXPECT_EQ(2, Cache.getTokenSize());
  EXPECT_EQ(1, Cache.getNodeSize());
}

TEST(GreenCacheTest, GetNodeLeftoverChildren) {
  auto Cache = GreenCache(kMaxCachedNodeSize);

  const GreenCache::Entry Entry1 =
      Cache.getToken(kTestSyntaxKindZero, kTestSource1);

  const GreenCache::Entry Entry2 =
      Cache.getToken(kTestSyntaxKindOne, kTestSource2);

  auto Children = std::vector{Entry1, Entry2};
  const auto Entry = Cache.getNode(kTestSyntaxKindThree, &Children, 1);

  // The node should have two children.
  EXPECT_EQ(1, Entry.Element.tryGetNode()->getChildren().size());

  // Children vector should have its elements reduced.
  EXPECT_EQ(1, Children.size());

  // One in the cache, one held in this test method.
  EXPECT_EQ(2, Entry.Element.getUseCount());

  // Two different tokens for Entry1.Element, and Entry2.Element. One node
  // for node.
  EXPECT_EQ(2, Cache.getTokenSize());
  EXPECT_EQ(1, Cache.getNodeSize());
}

TEST(GreenCacheTest, GetNodeDuplicateNodes) {
  auto Cache = GreenCache(kMaxCachedNodeSize);

  const GreenCache::Entry Child1 =
      Cache.getToken(kTestSyntaxKindZero, kTestSource1);

  const GreenCache::Entry Child2 =
      Cache.getToken(kTestSyntaxKindZero, kTestSource1);

  auto Children = std::vector{Child1, Child2};

  const auto Entry1 = Cache.getNode(kTestSyntaxKindThree, &Children, 1);
  const auto Entry2 = Cache.getNode(kTestSyntaxKindThree, &Children, 0);

  // Children vector should have its elements removed.
  EXPECT_EQ(0, Children.size());

  // One token for Entry1.Element and Entry2.Element. One node for
  // Entry1.Element and Entry2.Element.
  EXPECT_EQ(1, Cache.getTokenSize());
  EXPECT_EQ(1, Cache.getNodeSize());

  // Hashes for the same node should be the same.
  EXPECT_EQ(Entry1.Hash, Entry2.Hash);

  // The node should have two children.
  EXPECT_EQ(1, Entry1.Element.tryGetNode()->getChildren().size());
  EXPECT_EQ(1, Entry2.Element.tryGetNode()->getChildren().size());

  // One in the cache, two held in this test method since the ndoes are the
  // same.
  EXPECT_EQ(3, Entry1.Element.getUseCount());
  EXPECT_EQ(3, Entry2.Element.getUseCount());
}

TEST(GreenCacheTest, GetNodeDuplicateNodesOverMaxCacheSize) {
  auto Cache = GreenCache(0);

  const GreenCache::Entry Child1 =
      Cache.getToken(kTestSyntaxKindZero, kTestSource1);

  const GreenCache::Entry Child2 =
      Cache.getToken(kTestSyntaxKindZero, kTestSource1);

  auto Children = std::vector{Child1, Child2};

  const auto Entry1 = Cache.getNode(kTestSyntaxKindThree, &Children, 1);
  const auto Entry2 = Cache.getNode(kTestSyntaxKindThree, &Children, 0);

  // Children vector should have its elements removed.
  EXPECT_EQ(0, Children.size());

  // One token for Entry1.Element and Entry2.Element, however no nodes
  // should be cached.
  EXPECT_EQ(1, Cache.getTokenSize());
  EXPECT_EQ(0, Cache.getNodeSize());

  // Hashes for the same node should be the same.
  EXPECT_EQ(0, Entry1.Hash);
  EXPECT_EQ(0, Entry2.Hash);

  // The node should have two children.
  EXPECT_EQ(1, Entry1.Element.tryGetNode()->getChildren().size());
  EXPECT_EQ(1, Entry2.Element.tryGetNode()->getChildren().size());

  // At this point, each node is *not* cached.
  EXPECT_EQ(1, Entry1.Element.getUseCount());
  EXPECT_EQ(1, Entry2.Element.getUseCount());
}
} // namespace
