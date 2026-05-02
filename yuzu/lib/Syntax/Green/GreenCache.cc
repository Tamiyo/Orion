#include "yuzu/Syntax/Green/GreenCache.h"

#include "yuzu/Syntax/Green/Green.h"
#include "yuzu/Syntax/Green/GreenIterator.h"
#include "yuzu/Syntax/SyntaxKind.h"

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

namespace yuzu::syntax {

namespace {
constexpr unsigned int hashConstant = 0x9e3779b9;
} // namespace

// ---------------------- Tokens -----------------------

GreenCacheEntry GreenCache::getToken(const SyntaxKind kind,
                                     const std::u32string_view &source) {
  const size_t hash = hashToken(kind, source);

  const auto it = tokens.find(hash);
  if (it != tokens.end()) {
    return GreenCacheEntry{.hash = hash, .element = it->second};
  }

  const auto token = GreenToken(kind, std::move(source));

  tokens.emplace(hash, std::move(token));

  return GreenCacheEntry{.hash = hash, .element = tokens.at(hash)};
}

size_t GreenCache::hashToken(const SyntaxKind kind,
                             const std::u32string_view &source) const {
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

  const auto it = nodes.find(hash);
  if (it != nodes.end()) {
    const auto &cachedElement = it->second;

    std::vector<GreenElement> entryElements;
    entryElements.reserve(childrenSize);

    for (size_t i = firstChild; i < children->size(); ++i) {
      entryElements.emplace_back(std::move(children->at(i).element));
    }

    if (const GreenNode *entryNode = cachedElement.getIfNode()) {
      const bool isSameKinds = entryNode->getKind() == kind;

      const bool isSameChildren = std::equal(
          entryNode->getChildren().begin(), entryNode->getChildren().end(),
          entryElements.begin(), entryElements.end(),
          [](const GreenChild &child, const GreenElement &element) {
            return child.element == element;
          });

      if (isSameKinds && isSameChildren) {
        children->erase(children->begin() + firstChild, children->end());
        return GreenCacheEntry{hash, cachedElement};
      }
    }
  }

  const GreenNode node = buildNode(kind, children, firstChild);
  nodes.emplace(hash, std::move(node));

  return GreenCacheEntry{hash, nodes.at(hash)};
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
