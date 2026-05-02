#ifndef YUZU_SYNTAX_GREEN_GREEN_CACHE_H
#define YUZU_SYNTAX_GREEN_GREEN_CACHE_H

#include "yuzu/Syntax/Green/Green.h"
#include "yuzu/Syntax/SyntaxKind.h"

#include <cstddef>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace yuzu::syntax {
/// \brief A cache entry containing a hashed green element.
///
/// GreenCacheEntry pairs a GreenElement (node or token) with its hash value
/// for efficient lookup and deduplication in the cache. The hash is computed
/// based on the element's kind and structure.
struct [[nodiscard]] GreenCacheEntry {
  /// The hash value of the element, used for cache lookup.
  size_t hash;

  /// The cached green element (either a node or token).
  GreenElement element;
};

/// \brief Cache for deduplicating green tree nodes and tokens.
///
/// GreenCache implements structural sharing by maintaining hash-based caches
/// of GreenNodes and GreenTokens. When creating a new element, the cache
/// checks if an equivalent element already exists and returns it if found,
/// reducing memory usage.
///
/// The cache uses size limits to control which nodes are cached. Nodes with
/// more children than the configured maximum are created but not cached,
/// trading some memory savings for construction speed.
class [[nodiscard]] GreenCache final {
public:
  /// \brief Construct a GreenCache with specified size limit.
  ///
  /// \param maxCachedNodeSize The maximum number of children a node can have
  /// to be eligible for caching. Larger nodes will be created but not cached.
  explicit GreenCache(const size_t maxCachedNodeSize)
      : maxCachedNodeSize(maxCachedNodeSize) {}

  /// Deleted default constructor to enforce configuration.
  GreenCache() = delete;

  /// \brief Get or create a cached GreenNode.
  ///
  /// Attempts to find an existing node with the same structure in the cache.
  /// If found, returns the cached node and removes the children from the
  /// children vector. If not found, builds a new node and caches it (if it
  /// meets the size criteria).
  ///
  /// \param kind The syntax kind of the node.
  /// \param children Pointer to the vector of child cache entries.
  /// \param firstChild The index of the first child in the children vector.
  /// \return A GreenCacheEntry containing the node and its hash.
  GreenCacheEntry getNode(const SyntaxKind kind,
                          std::vector<GreenCacheEntry> *children,
                          const size_t firstChild) noexcept;

  /// \brief Get or create a cached GreenToken.
  ///
  /// Attempts to find an existing token with the same kind and source in the
  /// cache. If found, returns the cached token. If not found, creates a new
  /// token and adds it to the cache.
  ///
  /// \param kind The syntax kind of the token.
  /// \param source The source text content of the token.
  /// \return A GreenCacheEntry containing the token and its hash.
  GreenCacheEntry getToken(const SyntaxKind kind,
                           const std::u32string_view &source) noexcept;

  /// \brief Get the number of cached nodes.
  ///
  /// \return The size of the nodes cache.
  [[nodiscard]] size_t getNodeSize() const noexcept { return nodes.size(); }

  /// \brief Get the number of cached tokens.
  ///
  /// \return The size of the tokens cache.
  [[nodiscard]] size_t getTokenSize() const noexcept { return tokens.size(); }

private:
  /// \brief Compute hash for a node based on its structure.
  ///
  /// Combines the node's kind with the hashes of its children to produce
  /// a hash value for cache lookup.
  ///
  /// \param kind The syntax kind of the node.
  /// \param children The vector of child cache entries.
  /// \param firstChild The index of the first child to include in the hash.
  /// \return The computed hash value, or 0 if the node is not cacheable.
  [[nodiscard]] size_t hashNode(const SyntaxKind kind,
                                const std::vector<GreenCacheEntry> &children,
                                const size_t firstChild) const noexcept;

  /// \brief Compute hash for a token based on its kind and source.
  ///
  /// \param kind The syntax kind of the token.
  /// \param source The source text content of the token.
  /// \return The computed hash value.
  [[nodiscard]] size_t
  hashToken(const SyntaxKind kind,
            const std::u32string_view &source) const noexcept;

  /// \brief Build a new GreenNode from child entries.
  ///
  /// Constructs a GreenNode by extracting elements from the children vector
  /// starting at firstChild, then removes those children from the vector.
  ///
  /// \param kind The syntax kind of the node.
  /// \param children Pointer to the vector of child cache entries.
  /// \param firstChild The index of the first child in the children vector.
  /// \return The newly constructed GreenNode.
  GreenNode buildNode(const SyntaxKind kind,
                      std::vector<GreenCacheEntry> *children,
                      const size_t firstChild) const noexcept;

  /// Maximum number of children a node can have to be cached.
  const size_t maxCachedNodeSize;

  /// Cache of deduplicated nodes, keyed by structural hash.
  /// TODO(tamiyo): These should probably be a form of set with a custom
  /// hashing function for performance.
  std::unordered_map<size_t, GreenElement> nodes;

  /// Cache of deduplicated tokens, keyed by content hash.
  std::unordered_map<size_t, GreenElement> tokens;
};

} // namespace yuzu::syntax

#endif // YUZU_SYNTAX_GREEN_GREEN_CACHE_H
