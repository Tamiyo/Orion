#ifndef SYNTAX_GREEN_GREEN_BUILDER_H
#define SYNTAX_GREEN_GREEN_BUILDER_H

#include "Syntax/Green/GreenCache.h"
#include "Syntax/SyntaxKind.h"

#include <cstddef>
#include <string>
#include <vector>

namespace yuzu::syntax {
struct GreenBuilderCheckpoint {
  const size_t Index;
};

class GreenBuilder {
public:
  explicit GreenBuilder();
  explicit GreenBuilder(const size_t MaxNodeSize);

  void startNode(const SyntaxKind Kind) noexcept;

  void finishNode() noexcept;

  void startNodeAt(const GreenBuilderCheckpoint &Checkpoint,
                   const SyntaxKind Kind) noexcept;

  [[nodiscard]] GreenBuilderCheckpoint checkpoint() const noexcept;

  void token(const SyntaxKind Kind, const std::u32string &Source) noexcept;

  [[nodiscard]] GreenNode finish() noexcept;

  [[nodiscard]] size_t getParentsSize() const noexcept {
    return Parents_.size();
  }

  [[nodiscard]] size_t getChildrenSize() const noexcept {
    return Children_.size();
  }

private:
  struct Parent {
    const SyntaxKind Kind;
    const size_t FirstChild;
  };

  GreenCache Cache_;
  std::vector<GreenBuilder::Parent> Parents_;
  std::vector<GreenCache::Entry> Children_;
};

} // namespace yuzu::syntax

#endif // SYNTAX_GREEN_GREEN_BUILDER_H
