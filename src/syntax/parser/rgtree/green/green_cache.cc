#include "syntax/parser/rgtree/green/green_cache.h"

#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include "syntax/parser/rgtree/green/green_element.h"
#include "syntax/parser/rgtree/green/green_node.h"
#include "syntax/parser/rgtree/green/green_token.h"
#include "syntax/parser/syntax_kind.h"

// https://github.com/CAD97/sorbus/tree/main/src/green
// https://github.com/rust-analyzer/rowan/tree/master/src/green
namespace orion::syntax {
namespace {
size_t HashToken(const SyntaxKind kind, const std::u32string& source) noexcept {
  size_t hash_value = std::hash<SyntaxKind>{}(kind);
  hash_value ^= std::hash<std::u32string>{}(source) + 0x9e3779b9 +
                (hash_value << 6) + (hash_value >> 2);

  return hash_value;
}

size_t HashNode(const SyntaxKind kind,
                const std::vector<CachedGreenElement>& children,
                const size_t first_child) noexcept {
  size_t hash_value = std::hash<SyntaxKind>{}(kind);

  for (auto begin_iter = children.begin() + static_cast<long>(first_child);
       begin_iter != children.end(); ++begin_iter) {
    const auto& [h, _] = *begin_iter;
    if (h == 0) {
      return 0;
    }
    hash_value ^= h;
  }
  hash_value += 0x9e3779b9 + (hash_value << 6) + (hash_value >> 2);

  return hash_value;
}

GreenNode BuildNode(const SyntaxKind kind,
                    std::vector<CachedGreenElement>& children,
                    const size_t first_child) noexcept {
  const size_t size = children.size() - first_child;

  // Move children into the node allocation, removing old children in the
  // process.
  std::vector<GreenElement> elements;
  elements.reserve(size);
  std::ranges::move(children | std::views::drop(first_child) |
                        std::views::transform([](CachedGreenElement& cached) {
                          return cached.element;
                        }),
                    std::back_inserter(elements));

  // Since the children have been moved, the data in children between
  // [first_child, size] is garbage and needs to be cleaned up.
  children.erase(children.begin() + static_cast<long>(first_child),
                 children.end());

  return GreenNode(kind, elements);
}
}  // namespace

CachedGreenElement GreenCache::GetNode(
    const SyntaxKind kind, std::vector<CachedGreenElement>& children,
    const size_t first_child) {
  // If the number of children is greater than some value (determined
  // heuristically), then it's cheaper to just construct a new node.
  const size_t size = children.size() - first_child;
  if (size > max_cached_node_size_) {
    const auto node = BuildNode(kind, children, first_child);
    return {0, node};
  }

  // Compute the hash of the node.
  const size_t hash = HashNode(kind, children, first_child);
  const auto no_hash = NoHash{hash, GreenElement()};

  // If the entry exists, then there might be a collision.
  if (const auto entry = nodes_.find(no_hash); entry != nodes_.end()) {
    std::vector<GreenElement> entry_elements;
    entry_elements.reserve(size);

    // Unlike BuildNode, we do not know if we should erase these children yet.
    // Instead, we copy the children.
    std::ranges::copy(children | std::views::drop(first_child) |
                          std::views::transform([](CachedGreenElement& cached) {
                            return cached.element;
                          }),
                      std::back_inserter(entry_elements));

    // If the entry is the same as what we are trying to build, we should just
    // used the cached node.
    if (const std::optional<GreenNode> entry_node = entry->element.TryGetNode();
        entry_node.has_value() && entry_node->Kind() == kind &&
        entry_node->Children().size() == size &&
        entry_node->Children() == entry_elements) {
      // If the node already exists, then we can rease the children
      // that "would have been" included in the new node.
      children.erase(children.begin() + static_cast<long>(first_child),
                     children.end());

      return {entry->hash, entry->element};
    }
  }

  // Otherwise, if the entry is not present then we insert an new node into the
  // cache and return a copied reference.
  const auto node = BuildNode(kind, children, first_child);
  nodes_.insert({hash, node});
  return {hash, node};
}

CachedGreenElement GreenCache::GetToken(const SyntaxKind kind,
                                        const std::u32string& source) {
  const size_t hash_value = HashToken(kind, source);
  const auto token = GreenToken(kind, source);

  const auto no_hash = NoHash{hash_value, token};
  auto [entry, _] = tokens_.insert(no_hash);

  return {hash_value, entry->element};
}
}  // namespace orion::syntax