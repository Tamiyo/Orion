#ifndef SYNTAX_GREEN_GREEN_BUILDER_H_
#define SYNTAX_GREEN_GREEN_BUILDER_H_

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "syntax/green/green_cache.h"
#include "syntax/syntax_kind.h"

namespace yuzu::syntax {
struct GreenBuilderCheckpoint {
  const size_t index;
};

class GreenBuilder {
 public:
  explicit GreenBuilder();
  explicit GreenBuilder(const size_t maxNodeSize);

  void StartNode(const SyntaxKind kind) noexcept;

  void FinishNode();

  void StartNodeAt(const GreenBuilderCheckpoint& checkpoint,
                   const SyntaxKind kind);

  [[nodiscard]] GreenBuilderCheckpoint Checkpoint() const noexcept;

  void Token(const SyntaxKind kind, const std::u32string& source) noexcept;

  [[nodiscard]] GreenNode Finish();

  [[nodiscard]] size_t ParentsSize() const noexcept { return parents_.size(); }

  [[nodiscard]] size_t ChildrenSize() const noexcept {
    return children_.size();
  }

 private:
  struct Parent {
    const SyntaxKind kind;
    const size_t first_child;
  };

  std::vector<GreenBuilder::Parent> parents_;
  std::vector<GreenCache::Entry> children_;
  GreenCache cache_;
};

}  // namespace yuzu::syntax

#endif  // SYNTAX_GREEN_GREEN_BUILDER_H_
