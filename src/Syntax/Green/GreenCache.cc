#include "Syntax/Green/GreenCache.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "Syntax/Green/Green.h"
#include "Syntax/SyntaxKind.h"

namespace yuzu::syntax {

namespace {
constexpr unsigned int kHashConstant = 0x9e3779b9;
} // namespace

// ---------------------- Tokens -----------------------

GreenCache::Entry GreenCache::getToken(const SyntaxKind Kind,
                                       const std::u32string &Source) noexcept {
  const size_t Hash = hashToken(Kind, Source);

  auto It = Tokens_.find(Hash);
  if (It != Tokens_.end())
    return Entry{Hash, It->second};

  auto Token = GreenToken(Kind, Source);
  auto Element = GreenElement(Token);

  // Use emplace to avoid copy assignment.
  Tokens_.emplace(Hash, std::move(Element));

  // Return a GreenCache::Entry with a copy/move of the element.
  return Entry{Hash, Tokens_.at(Hash)};
}

size_t GreenCache::hashToken(const SyntaxKind Kind,
                             const std::u32string &Source) const noexcept {
  size_t Hash = std::hash<uint16_t>{}(Kind.Value);
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

    for (size_t I = FirstChild; I < Children->size(); ++I)
      EntryElements.push_back(Children->at(I).Element);

    if (std::optional<GreenNode> EntryNodeOpt = CachedElement.tryGetNode();
        EntryNodeOpt != std::nullopt && EntryNodeOpt->getKind() == Kind &&
        EntryNodeOpt->getChildren().size() == ChildrenSize &&
        EntryNodeOpt->getChildren() == EntryElements) {
      Children->erase(Children->begin() + FirstChild, Children->end());
      return Entry{Hash, CachedElement};
    }
  }

  auto Node = buildNode(Kind, Children, FirstChild);
  GreenElement Element(Node);

  // Use emplace to insert move-only element.
  Nodes_.emplace(Hash, std::move(Element));

  return Entry{Hash, Nodes_.at(Hash)};
}

size_t GreenCache::hashNode(const SyntaxKind Kind,
                            const std::vector<Entry> &Children,
                            const size_t FirstChild) const noexcept {
  size_t Hash = std::hash<uint16_t>{}(Kind.Value);

  for (size_t I = FirstChild; I < Children.size(); ++I) {
    const size_t ChildHash = Children[I].Hash;
    if (ChildHash == 0)
      return 0;
    Hash ^= ChildHash + kHashConstant + (Hash << 6) + (Hash >> 2);
  }

  return Hash;
}

GreenNode GreenCache::buildNode(const SyntaxKind Kind,
                                std::vector<Entry> *Children,
                                const size_t FirstChild) const noexcept {
  std::vector<GreenElement> Elements;
  Elements.reserve(Children->size() - FirstChild);

  for (size_t I = FirstChild; I < Children->size(); ++I)
    Elements.push_back(std::move(Children->at(I).Element));

  Children->erase(Children->begin() + FirstChild, Children->end());

  return GreenNode(Kind, Elements);
}

} // namespace yuzu::syntax
