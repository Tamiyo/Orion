#ifndef YUZU_SYNTAX_GREEN_GREEN_BUILDER_H
#define YUZU_SYNTAX_GREEN_GREEN_BUILDER_H

#include "yuzu/Syntax/Green/GreenCache.h"
#include "yuzu/Syntax/SyntaxKind.h"

#include <cstddef>
#include <string_view>
#include <vector>

namespace yuzu::syntax {
struct GreenBuilderCheckpoint {
  const size_t index;
};

class GreenBuilder final {
public:
  explicit GreenBuilder();
  explicit GreenBuilder(const size_t maxCachedNodeSize);

  void startNode(const SyntaxKind kind) noexcept;

  void finishNode() noexcept;

  void startNodeAt(const GreenBuilderCheckpoint &checkpoint,
                   const SyntaxKind kind) noexcept;

  [[nodiscard]] GreenBuilderCheckpoint checkpoint() const noexcept;

  void token(const SyntaxKind kind, const std::u32string_view &source) noexcept;

  [[nodiscard]] GreenNode finish() noexcept;

  [[nodiscard]] size_t getParentsSize() const noexcept {
    return parents.size();
  }

  [[nodiscard]] size_t getChildrenSize() const noexcept {
    return children.size();
  }

private:
  struct Parent {
    const SyntaxKind kind;
    const size_t firstChild;
  };

  GreenCache cache;
  std::vector<GreenBuilder::Parent> parents;
  std::vector<GreenCacheEntry> children;
};

} // namespace yuzu::syntax

#endif // YUZU_SYNTAX_GREEN_GREEN_BUILDER_H
