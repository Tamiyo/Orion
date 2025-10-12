#include "Syntax/Green/GreenCache.h"

#include "Syntax/Green/Green.h"
#include "Syntax/SyntaxKind.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace yuzu::syntax {

namespace {
constexpr unsigned int kHashConstant = 0x9e3779b9;
} // namespace

// ---------------------- Tokens -----------------------

GreenCache::Entry GreenCache::getToken(const SyntaxKind Kind,
                                       const std::u32string &Source) noexcept {
  const size_t Hash = hashToken(Kind, Source);

  auto It = Tokens_.find(Hash);
  if (It != Tokens_.end()) {
    return Entry{Hash, It->second};
  }

  const auto Token = GreenToken(Kind, Source);

  // Use emplace to avoid copy assignment.
  Tokens_.emplace(Hash, std::move(Token));

  // Return a GreenCache::Entry with a copy/move of the element.
  return Entry{Hash, Tokens_.at(Hash)};
}

size_t GreenCache::hashToken(const SyntaxKind Kind,
                             const std::u32string &Source) const noexcept {
  size_t Hash = std::hash<uint16_t>{}(Kind);
  Hash ^= std::hash<std::u32string>{}(Source) + kHashConstant + (Hash << 6) +
          (Hash >> 2);
  return Hash;
}

// ---------------------- Nodes -----------------------

GreenCache::Entry GreenCache::getNode(const SyntaxKind Kind,
                                      std::vector<Entry> *Children,
                                      const size_t FirstChild) noexcept {
  const size_t ChildrenSize = Children->size() - FirstChild;

  if (ChildrenSize > MaxCachedNodeSize_) {
    auto Node = buildNode(Kind, Children, FirstChild);
    return Entry{0, GreenElement(Node)};
  }

  const size_t Hash = hashNode(Kind, *Children, FirstChild);

  auto It = Nodes_.find(Hash);
  if (It != Nodes_.end()) {
    const auto &CachedElement = It->second;

    std::vector<GreenElement> EntryElements;
    EntryElements.reserve(ChildrenSize);

    for (size_t I = FirstChild; I < Children->size(); ++I) {
      EntryElements.emplace_back(std::move(Children->at(I).Element));
    }

    if (const GreenNode *EntryNode = CachedElement.getIfNode();
        EntryNode != nullptr && EntryNode->getKind() == Kind &&
        std::equal(EntryNode->getChildren().begin(),
                   EntryNode->getChildren().end(), EntryElements.begin(),
                   EntryElements.end())) {
      Children->erase(Children->begin() + FirstChild, Children->end());
      return Entry{Hash, CachedElement};
    }
  }

  auto Node = buildNode(Kind, Children, FirstChild);

  // Use emplace to insert move-only element.
  Nodes_.emplace(Hash, std::move(Node));

  return Entry{Hash, Nodes_.at(Hash)};
}

size_t GreenCache::hashNode(const SyntaxKind Kind,
                            const std::vector<Entry> &Children,
                            const size_t FirstChild) const noexcept {
  size_t Hash = std::hash<uint16_t>{}(Kind);

  for (size_t I = FirstChild; I < Children.size(); ++I) {
    const size_t ChildHash = Children[I].Hash;
    if (ChildHash == 0) {
      return 0;
    }
    Hash ^= ChildHash + kHashConstant + (Hash << 6) + (Hash >> 2);
  }

  return Hash;
}

GreenNode GreenCache::buildNode(const SyntaxKind Kind,
                                std::vector<Entry> *Children,
                                const size_t FirstChild) const noexcept {
  std::vector<GreenElement> Elements;
  Elements.reserve(Children->size() - FirstChild);

  for (size_t I = FirstChild; I < Children->size(); ++I) {
    Elements.emplace_back(std::move(Children->at(I).Element));
  }

  Children->erase(Children->begin() + FirstChild, Children->end());

  return GreenNode::create(Kind, std::move(Elements));
}

} // namespace yuzu::syntax
