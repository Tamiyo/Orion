#ifndef SYNTAX_GREEN_GREEN_CACHE_H_
#define SYNTAX_GREEN_GREEN_CACHE_H_

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "syntax/green/green_element.h"
#include "syntax/green/green_node.h"
#include "syntax/green/green_token.h"
#include "syntax/syntax_kind.h"

namespace yuzu::syntax {

class GreenCache {
 public:
  struct Entry {
    size_t hash;
    GreenElement element;
  };

  explicit GreenCache(const size_t max_cached_node_size)
      : max_cached_node_size_(max_cached_node_size) {}

  GreenCache() = delete;

  [[nodiscard]] Entry GetNode(const SyntaxKind kind,
                              std::vector<Entry>* children,
                              const size_t first_child) noexcept;

  [[nodiscard]] Entry GetToken(const SyntaxKind kind,
                               const std::u32string& source) noexcept;

  [[nodiscard]] size_t NodeSize() const noexcept { return nodes_.size(); }

  [[nodiscard]] size_t TokenSize() const noexcept { return tokens_.size(); }

 private:
  [[nodiscard]] size_t HashNode(const SyntaxKind kind,
                                const std::vector<Entry>& children,
                                const size_t first_child) const noexcept;

  [[nodiscard]] size_t HashToken(const SyntaxKind kind,
                                 const std::u32string& source) const noexcept;

  [[nodiscard]] GreenNode BuildNode(const SyntaxKind kind,
                                    std::vector<Entry>* children,
                                    const size_t first_child) const noexcept;

  const size_t max_cached_node_size_;

  // todo(tamiyo) these should probably be a form of set with a custom hashing
  // function for performance
  std::unordered_map<size_t, GreenElement> nodes_;
  std::unordered_map<size_t, GreenElement> tokens_;
};

}  // namespace yuzu::syntax

#endif  // SYNTAX_GREEN_GREEN_CACHE_H_
