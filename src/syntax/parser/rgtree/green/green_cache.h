#ifndef SYNTAX_PARSER_RGTREE_GREEN_GREEN_CACHE_H_
#define SYNTAX_PARSER_RGTREE_GREEN_GREEN_CACHE_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include "syntax/parser/rgtree/green/green.h"

namespace yuzu::syntax {
/// \brief Caches green nodes and tokens for efficient reuse.
///
/// The `GreenCache` class manages a cache of `GreenNode` and `GreenToken`
/// objects, allowing for quick retrieval and preventing unnecessary allocations
/// during parsing.
template <typename SyntaxKind = uint16_t>
class GreenCache {
 private:
  using GreenElement = GreenElement<SyntaxKind>;
  using GreenNode = GreenNode<SyntaxKind>;

 public:
  /// \brief Represents a cached green element with its corresponding hash.
  ///
  /// The `Cached` struct is used to store a hash value along with
  /// the associated `GreenElement`, allowing for efficient caching and lookup.
  class Cached {
   public:
    explicit Cached(const size_t hash, GreenElement element)
        : hash_(hash), element_(std::move(element)) {}

    /// \brief Deleted default constructor.
    ///
    /// A `Cached` must always be constructed explicitly.
    Cached() = delete;

    /// \brief Retrieves the hash value of the cached green element.
    ///
    /// \return The hash value.
    [[nodiscard]] size_t Hash() const { return this->hash_; }

    /// \brief Retrieves the cached green element.
    ///
    /// \return The `GreenElement` stored in this cache.
    [[nodiscard]] const GreenElement& Element() const {
      return this->element_;
    }

   private:
    size_t hash_;
    GreenElement element_;
  };

  /// \brief Constructs a `GreenCache` with a specified maximum size for cached
  /// nodes.
  ///
  /// \param max_cached_node_size The maximum number of nodes to cache.
  explicit GreenCache(const size_t max_cached_node_size)
      : max_cached_node_size_(max_cached_node_size), nodes_({}), tokens_({}) {}

  /// \brief Deleted default constructor.
  ///
  /// A `GreenCache` must always be constructed with a maximum cached node size.
  GreenCache() = delete;

  /// \brief Retrieves a cached node based on its kind and child elements.
  ///
  /// \param kind The kind of the node as defined by `SyntaxKind`.
  /// \param children The vector of child `Cached`s.
  /// \param first_child The index of the first child element.
  /// \return A `Cached` containing the cached node.
  [[nodiscard]] Cached GetNode(const SyntaxKind kind,
                               std::vector<Cached>* children,
                               const size_t first_child) {
    // If the number of children is greater than some value (determined
    // heuristically), then it's cheaper to just construct a new node.
    const size_t size = children->size() - first_child;
    if (size > max_cached_node_size_) {
      const auto node = BuildNode(kind, children, first_child);
      return Cached(0, GreenElement(node));
    }

    // Compute the hash of the node.
    const size_t hash = HashNode(kind, *children, first_child);
    const auto no_hash = NoHash{hash, GreenElement()};

    // If the entry exists, then there might be a collision.
    if (const auto entry = nodes_.find(no_hash); entry != nodes_.end()) {
      std::vector<GreenElement> entry_elements;
      entry_elements.reserve(size);

      // Unlike BuildNode, we do not know if we should erase these children yet.
      // Instead, we copy the children.
      std::ranges::copy(*children | std::views::drop(first_child) |
                            std::views::transform([](Cached& cached) {
                              return cached.Element();
                            }),
                        std::back_inserter(entry_elements));

      // If the entry is the same as what we are trying to build, we should just
      // used the cached node.
      if (const std::optional<GreenNode> entry_node =
              entry->element.TryGetNode();
          entry_node.has_value() && entry_node->Kind() == kind &&
          entry_node->Children().size() == size &&
          entry_node->Children() == entry_elements) {
        // If the node already exists, then we can rease the children
        // that "would have been" included in the new node.
        children->erase(children->begin() + first_child, children->end());

        return Cached(entry->hash, entry->element);
      }
    }

    // Otherwise, if the entry is not present then we insert an new node into
    // the cache and return a copied reference.
    const auto node = BuildNode(kind, children, first_child);
    const auto element = GreenElement(node);
    nodes_.insert({hash, element});
    return Cached(hash, element);
  }

  /// \brief Retrieves a cached token based on its kind and source text.
  ///
  /// \param kind The kind of the token as defined by `SyntaxKind`.
  /// \param source The source text of the token.
  /// \return A `Cached` containing the cached token.
  [[nodiscard]] Cached GetToken(const SyntaxKind kind,
                                std::u32string_view source) {
    const size_t hash_value = HashToken(kind, source);
    const auto token = GreenToken(kind, source);

    const auto no_hash = NoHash{hash_value, GreenElement(token)};
    auto [entry, _] = tokens_.insert(no_hash);

    return Cached(hash_value, entry->element);
  }

  /// \brief Returns the current size of the cached nodes.
  ///
  /// \return The number of cached nodes.
  [[nodiscard]] size_t NodeSize() const noexcept { return nodes_.size(); }

  /// \brief Returns the current size of the cached tokens.
  ///
  /// \return The number of cached tokens.
  [[nodiscard]] size_t TokenSize() const noexcept { return tokens_.size(); }

 private:
  [[nodiscard]] size_t HashToken(const SyntaxKind kind,
                                 std::u32string_view source) noexcept {
    size_t hash_value = std::hash<SyntaxKind>{}(kind);
    hash_value ^= std::hash<std::u32string_view>{}(source) + 0x9e3779b9 +
                  (hash_value << 6) + (hash_value >> 2);

    return hash_value;
  }

  [[nodiscard]] size_t HashNode(const SyntaxKind kind,
                                const std::vector<Cached>& children,
                                const size_t first_child) noexcept {
    size_t hash_value = std::hash<SyntaxKind>{}(kind);

    for (auto begin_iter = children.begin() + first_child;
         begin_iter != children.end(); ++begin_iter) {
      const auto& value = *begin_iter;
      const auto hash = value.Hash();
      if (hash == 0) {
        return 0;
      }
      hash_value ^= hash;
    }
    hash_value += 0x9e3779b9 + (hash_value << 6) + (hash_value >> 2);

    return hash_value;
  }

  GreenNode BuildNode(const SyntaxKind kind,
                                  std::vector<Cached>* children,
                                  const size_t first_child) noexcept {
    const size_t size = children->size() - first_child;

    // Move children into the node allocation, removing old children in the
    // process.
    std::vector<GreenElement> elements;
    elements.reserve(size);
    std::ranges::move(*children | std::views::drop(first_child) |
                          std::views::transform(
                              [](Cached& cached) { return cached.Element(); }),
                      std::back_inserter(elements));

    // Since the children have been moved, the data in children between
    // [first_child, size] is garbage and needs to be cleaned up.
    children->erase(children->begin() + first_child, children->end());

    return GreenNode(kind, elements);
  }

  /// \brief Helper struct to store elements without a hash function.
  ///
  /// The `NoHash` struct is used in the cache to facilitate storage in
  /// unordered sets without relying on a hashing function for `GreenElement`.
  ///
  /// This structure, while similar, is distinctly different than
  /// `CachedGreenNode`. To prevent misuse this structure should be used
  /// internally instead of `CachedGreenNode`.
  struct NoHash {
    const size_t hash;

    /// The element being cached.
    const GreenElement element;

    /// \brief Compares two `NoHash` objects for equality.
    ///
    /// \param other The other `NoHash` to compare with.
    /// \return `true` if both hash values are equal, otherwise `false`.
    bool operator==(const NoHash& other) const noexcept {
      return hash == other.hash;
    }
  };

  /// \brief Custom hash function for `NoHash` objects.
  ///
  /// The `NoHashHasher` struct provides a way to obtain the hash value of a
  /// `NoHash` object for use in unordered sets.
  struct NoHashHasher {
    /// \brief Computes the hash of a `NoHash` key.
    ///
    /// \param key The `NoHash` object to hash.
    /// \return The hash value of the key.
    size_t operator()(const NoHash& key) const noexcept { return key.hash; }
  };

  const size_t max_cached_node_size_;
  std::unordered_set<NoHash, NoHashHasher> nodes_;
  std::unordered_set<NoHash, NoHashHasher> tokens_;
};

}  // namespace yuzu::syntax

#endif  // SYNTAX_PARSER_RGTREE_GREEN_GREEN_CACHE_H_
