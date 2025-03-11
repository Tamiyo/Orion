#ifndef SYNTAX_PARSER_RGTREE_GREEN_GREEN_CACHE_H_
#define SYNTAX_PARSER_RGTREE_GREEN_GREEN_CACHE_H_

#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "syntax/parser/rgtree/green/green_element.h"
#include "syntax/parser/rgtree/green/green_node.h"
#include "syntax/parser/rgtree/green/green_token.h"
#include "syntax/parser/syntax_kind.h"

namespace orion::syntax {

/**
 * @brief Represents a cached green element with its corresponding hash.
 *
 * The `CachedGreenElement` struct is used to store a hash value along with
 * the associated `GreenElement`, allowing for efficient caching and lookup.
 */
struct CachedGreenElement {
  /** The hash value associated with the green element. */
  const size_t hash;

  /** The cached green element (either a node or a token). */
  const GreenElement element;
};

/**
 * @brief Caches green nodes and tokens for efficient reuse.
 *
 * The `GreenCache` class manages a cache of `GreenNode` and `GreenToken`
 * objects, allowing for quick retrieval and preventing unnecessary allocations
 * during parsing.
 */
class GreenCache {
 public:
  /**
   * @brief Constructs a `GreenCache` with a specified maximum size for cached
   * nodes.
   *
   * @param max_cached_node_size The maximum number of nodes to cache.
   */
  explicit GreenCache(const size_t max_cached_node_size)
      : max_cached_node_size_(max_cached_node_size), nodes_({}), tokens_({}) {}

  /**
   * @brief Deleted default constructor.
   *
   * A `GreenCache` must always be constructed with a maximum cached node size.
   */
  GreenCache() = delete;

  /**
   * @brief Retrieves a cached node based on its kind and child elements.
   *
   * @param kind The kind of the node as defined by `SyntaxKind`.
   * @param children The vector of child `CachedGreenElement`s.
   * @param first_child The index of the first child element.
   * @return A `CachedGreenElement` containing the cached node.
   */
  [[nodiscard]] CachedGreenElement GetNode(
      SyntaxKind kind, std::vector<CachedGreenElement>& children,
      size_t first_child);

  /**
   * @brief Retrieves a cached token based on its kind and source text.
   *
   * @param kind The kind of the token as defined by `SyntaxKind`.
   * @param source The source text of the token.
   * @return A `CachedGreenElement` containing the cached token.
   */
  [[nodiscard]] CachedGreenElement GetToken(SyntaxKind kind,
                                            const std::u32string& source);

  /**
   * @brief Returns the current size of the cached nodes.
   *
   * @return The number of cached nodes.
   */
  [[nodiscard]] size_t NodeSize() const noexcept { return nodes_.size(); }

  /**
   * @brief Returns the current size of the cached tokens.
   *
   * @return The number of cached tokens.
   */
  [[nodiscard]] size_t TokenSize() const noexcept { return tokens_.size(); }

 private:
  /**
   * @brief Helper struct to store elements without a hash function.
   *
   * The `NoHash` struct is used in the cache to facilitate storage in
   * unordered sets without relying on a hashing function for `GreenElement`.
   *
   * This structure, while similar, is distinctly different than
   * `CachedGreenNode`. To prevent misuse this structure should be used
   * internally instead of `CachedGreenNode`.
   */
  struct NoHash {
    /** The hash value associated with the element. */
    const size_t hash;

    /**<The element being cached. */
    const GreenElement element;

    /**
     * @brief Compares two `NoHash` objects for equality.
     *
     * @param other The other `NoHash` to compare with.
     * @return `true` if both hash values are equal, otherwise `false`.
     */
    bool operator==(const NoHash& other) const noexcept {
      return hash == other.hash;
    }
  };

  /**
   * @brief Custom hash function for `NoHash` objects.
   *
   * The `NoHashHasher` struct provides a way to obtain the hash value of a
   * `NoHash` object for use in unordered sets.
   */
  struct NoHashHasher {
    /**
     * @brief Computes the hash of a `NoHash` key.
     *
     * @param key The `NoHash` object to hash.
     * @return The hash value of the key.
     */
    size_t operator()(const NoHash& key) const noexcept { return key.hash; }
  };

  /** The maximum number of children that can be cached before creating a new
   * node. */
  const size_t max_cached_node_size_;

  /** Set of cached nodes. */
  std::unordered_set<NoHash, NoHashHasher> nodes_;

  /** Set of cached tokens. */
  std::unordered_set<NoHash, NoHashHasher> tokens_;
};

}  // namespace orion::syntax

#endif  // SYNTAX
