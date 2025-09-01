#include "syntax/green/green_cache.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "syntax/green/green_element.h"
#include "syntax/green/green_node.h"
#include "syntax/green/green_token.h"
#include "syntax/syntax_kind.h"

namespace yuzu::syntax {

namespace {
constexpr unsigned int kHashConstant = 0x9e3779b9;
}  // namespace

// ---------------------- Tokens -----------------------

GreenCache::Entry GreenCache::GetToken(const SyntaxKind kind,
                                       const std::u32string& source) noexcept {
  const size_t hash = HashToken(kind, source);

  auto it = tokens_.find(hash);
  if (it != tokens_.end()) {
    return Entry{hash, it->second};
  }

  GreenToken token(kind, source);
  GreenElement element(token);

  // Use emplace to avoid copy assignment
  tokens_.emplace(hash, std::move(element));

  // Return a GreenCache::Entry with a copy/move of the element
  return Entry{hash, tokens_.at(hash)};
}

size_t GreenCache::HashToken(const SyntaxKind kind,
                             const std::u32string& source) const noexcept {
  size_t hash = std::hash<uint16_t>{}(kind.value);
  hash ^= std::hash<std::u32string>{}(source) + kHashConstant + (hash << 6) +
          (hash >> 2);
  return hash;
}

// ---------------------- Nodes -----------------------

GreenCache::Entry GreenCache::GetNode(const SyntaxKind kind,
                                      std::vector<Entry>* children,
                                      const size_t first_child) noexcept {
  const size_t children_size = children->size() - first_child;

  if (children_size > max_cached_node_size_) {
    const auto node = BuildNode(kind, children, first_child);
    return Entry{0, GreenElement(node)};
  }

  const size_t hash = HashNode(kind, *children, first_child);

  auto it = nodes_.find(hash);
  if (it != nodes_.end()) {
    const auto& cached_element = it->second;

    std::vector<GreenElement> entry_elements;
    entry_elements.reserve(children_size);

    for (size_t i = first_child; i < children->size(); ++i) {
      entry_elements.push_back(children->at(i).element);
    }

    if (auto entry_node_opt = cached_element.TryGetNode();
        entry_node_opt.has_value() && entry_node_opt.value().Kind() == kind &&
        entry_node_opt.value().Children().size() == children_size &&
        entry_node_opt.value().Children() == entry_elements) {
      children->erase(children->begin() + first_child, children->end());
      return Entry{hash, cached_element};
    }
  }

  const auto node = BuildNode(kind, children, first_child);
  GreenElement element(node);

  // Use emplace to insert move-only element
  nodes_.emplace(hash, std::move(element));

  return Entry{hash, nodes_.at(hash)};
}

size_t GreenCache::HashNode(const SyntaxKind kind,
                            const std::vector<Entry>& children,
                            const size_t first_child) const noexcept {
  size_t hash = std::hash<uint16_t>{}(kind.value);

  for (size_t i = first_child; i < children.size(); ++i) {
    const size_t child_hash = children[i].hash;
    if (child_hash == 0) return 0;
    hash ^= child_hash + kHashConstant + (hash << 6) + (hash >> 2);
  }

  return hash;
}

GreenNode GreenCache::BuildNode(const SyntaxKind kind,
                                std::vector<Entry>* children,
                                const size_t first_child) const noexcept {
  std::vector<GreenElement> elements;
  elements.reserve(children->size() - first_child);

  for (size_t i = first_child; i < children->size(); ++i) {
    elements.push_back(std::move(children->at(i).element));
  }

  children->erase(children->begin() + first_child, children->end());

  return GreenNode(kind, elements);
}

}  // namespace yuzu::syntax
