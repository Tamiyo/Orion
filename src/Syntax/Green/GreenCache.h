#ifndef SYNTAX_GREEN_GREEN_CACHE_H
#define SYNTAX_GREEN_GREEN_CACHE_H

#include "Syntax/Green/Green.h"
#include "Syntax/SyntaxKind.h"

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace yuzu::syntax {

class GreenCache {
public:
  struct Entry {
    size_t Hash;
    GreenElement Element;
  };

  explicit GreenCache(const size_t MaxCachedNodeSize)
      : MaxCachedNodeSize_(MaxCachedNodeSize) {}

  GreenCache() = delete;

  [[nodiscard]] Entry getNode(const SyntaxKind Kind,
                              std::vector<Entry> *Children,
                              const size_t FirstChild) noexcept;

  [[nodiscard]] Entry getToken(const SyntaxKind Kind,
                               const std::u32string &Source) noexcept;

  [[nodiscard]] size_t getNodeSize() const noexcept { return Nodes_.size(); }

  [[nodiscard]] size_t getTokenSize() const noexcept { return Tokens_.size(); }

private:
  [[nodiscard]] size_t hashNode(const SyntaxKind Kind,
                                const std::vector<Entry> &Children,
                                const size_t FirstChild) const noexcept;

  [[nodiscard]] size_t hashToken(const SyntaxKind Kind,
                                 const std::u32string &Source) const noexcept;

  [[nodiscard]] GreenNode buildNode(const SyntaxKind Kind,
                                    std::vector<Entry> *Children,
                                    const size_t FirstChild) const noexcept;

  const size_t MaxCachedNodeSize_;

  // TODO(tamiyo): These should probably be a form of set with a custom
  // hashing function for performance.
  std::unordered_map<size_t, GreenElement> Nodes_;
  std::unordered_map<size_t, GreenElement> Tokens_;
};

} // namespace yuzu::syntax

#endif // SYNTAX_GREEN_GREEN_CACHE_H
