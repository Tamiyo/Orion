#ifndef YUZU_SYNTAX_GREEN_GREEN_CACHE_H
#define YUZU_SYNTAX_GREEN_GREEN_CACHE_H

#include "yuzu/Syntax/Green/Green.h"
#include "yuzu/Syntax/SyntaxKind.h"

#include <cstddef>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace yuzu::syntax {
struct GreenCacheEntry {
  size_t hash;
  GreenElement element;
};

class GreenCache final {
public:
  explicit GreenCache(const size_t maxCachedNodeSize)
      : maxCachedNodeSize(maxCachedNodeSize) {}

  GreenCache() = delete;

  [[nodiscard]] GreenCacheEntry getNode(const SyntaxKind kind,
                                        std::vector<GreenCacheEntry> *children,
                                        const size_t firstChild) noexcept;

  [[nodiscard]] GreenCacheEntry
  getToken(const SyntaxKind kind, const std::u32string_view &source) noexcept;

  [[nodiscard]] size_t getNodeSize() const noexcept { return nodes.size(); }

  [[nodiscard]] size_t getTokenSize() const noexcept { return tokens.size(); }

private:
  [[nodiscard]] size_t hashNode(const SyntaxKind kind,
                                const std::vector<GreenCacheEntry> &children,
                                const size_t firstChild) const noexcept;

  [[nodiscard]] size_t
  hashToken(const SyntaxKind kind,
            const std::u32string_view &source) const noexcept;

  [[nodiscard]] GreenNode buildNode(const SyntaxKind kind,
                                    std::vector<GreenCacheEntry> *children,
                                    const size_t firstChild) const noexcept;

  const size_t maxCachedNodeSize;

  // TODO(tamiyo): These should probably be a form of set with a custom
  // hashing function for performance.
  std::unordered_map<size_t, GreenElement> nodes;
  std::unordered_map<size_t, GreenElement> tokens;
};

} // namespace yuzu::syntax

#endif // YUZU_SYNTAX_GREEN_GREEN_CACHE_H
