#include "yuzu/Syntax/Green/GreenCache.h"

#include "yuzu/Syntax/Green/Green.h"
#include "yuzu/Syntax/Green/GreenIterator.h"
#include "yuzu/Syntax/SyntaxKind.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace yuzu::syntax {

namespace {
constexpr unsigned int hashConstant = 0x9e3779b9;
} // namespace

// ---------------------- Tokens -----------------------

GreenCacheEntry GreenCache::getToken(const SyntaxKind kind,
                                     std::u32string_view source) {
  const size_t hash = hashToken(kind, source);

  // Walk every entry in the bucket and verify structural equality. Without
  // this check a hash collision would return the wrong cached token, and
  // because we use a multimap rather than unordered_map a collision never
  // causes a fresh token to be silently dropped.
  const auto range = tokens.equal_range(hash);
  for (auto it = range.first; it != range.second; ++it) {
    const GreenToken *cached = it->second.getIfToken();
    if (cached != nullptr && cached->getKind() == kind &&
        cached->getSource() == source) {
      return GreenCacheEntry{.hash = hash, .element = it->second};
    }
  }

  const auto inserted =
      tokens.emplace(hash, GreenToken(kind, std::u32string(source)));
  return GreenCacheEntry{.hash = hash, .element = inserted->second};
}

size_t GreenCache::hashToken(const SyntaxKind kind,
                             std::u32string_view source) const {
  size_t hash = std::hash<uint16_t>{}(kind);
  hash ^= std::hash<std::u32string_view>{}(source) + hashConstant +
          (hash << 6) + (hash >> 2);
  return hash;
}

// ---------------------- Nodes -----------------------

GreenCacheEntry GreenCache::getNode(const SyntaxKind kind,
                                    std::vector<GreenCacheEntry> *children,
                                    const size_t firstChild) {
  const size_t childrenSize = children->size() - firstChild;

  if (childrenSize > maxCachedNodeSize) {
    const GreenNode node = buildNode(kind, children, firstChild);
    return GreenCacheEntry{.hash = 0, .element = node};
  }

  const size_t hash = hashNode(kind, *children, firstChild);

  // Look for a structurally-equal node in the bucket. We deliberately do NOT
  // move elements out of `children` during comparison: if the bucket walk
  // ends without a hit we need the children intact to hand off to
  // buildNode(). Comparing by reference against `children->at(i).element`
  // keeps them valid.
  const auto range = nodes.equal_range(hash);
  for (auto it = range.first; it != range.second; ++it) {
    const GreenNode *entryNode = it->second.getIfNode();
    if (entryNode == nullptr || entryNode->getKind() != kind ||
        entryNode->getNumChildren() != childrenSize) {
      continue;
    }

    const GreenChildren cachedChildren = entryNode->getChildren();
    const bool sameChildren = std::equal(
        cachedChildren.begin(), cachedChildren.end(),
        children->begin() + firstChild, children->end(),
        [](const GreenChild &cached, const GreenCacheEntry &entry) {
          return cached.element == entry.element;
        });

    if (sameChildren) {
      // Release the now-consumed children and return the cached node.
      children->erase(children->begin() + firstChild, children->end());
      return GreenCacheEntry{hash, it->second};
    }
  }

  // No cached match. buildNode moves the children out and erases them from
  // the builder's vector.
  const GreenNode node = buildNode(kind, children, firstChild);
  const auto inserted = nodes.emplace(hash, GreenElement(node));
  return GreenCacheEntry{hash, inserted->second};
}

size_t GreenCache::hashNode(const SyntaxKind kind,
                            const std::vector<GreenCacheEntry> &children,
                            const size_t firstChild) const {
  size_t hash = std::hash<uint16_t>{}(kind);

  for (size_t i = firstChild; i < children.size(); ++i) {
    const size_t childHash = children[i].hash;
    if (childHash == 0) {
      return 0;
    }
    hash ^= childHash + hashConstant + (hash << 6) + (hash >> 2);
  }

  return hash;
}

GreenNode GreenCache::buildNode(const SyntaxKind kind,
                                std::vector<GreenCacheEntry> *children,
                                const size_t firstChild) const {
  std::vector<GreenElement> elements;
  elements.reserve(children->size() - firstChild);

  for (size_t childIndex = firstChild; childIndex < children->size();
       ++childIndex) {
    elements.emplace_back(std::move(children->at(childIndex).element));
  }

  children->erase(children->begin() + firstChild, children->end());

  return GreenNode::create(kind, std::move(elements));
}

} // namespace yuzu::syntax
