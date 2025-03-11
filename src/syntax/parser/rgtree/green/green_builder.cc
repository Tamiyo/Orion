#include "syntax/parser/rgtree/green/green_builder.h"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

#include "syntax/parser/rgtree/green/green_cache.h"
#include "syntax/parser/rgtree/green/green_element.h"
#include "syntax/parser/syntax_kind.h"

// https://github.com/rust-analyzer/rowan/tree/master/src/green
namespace orion::syntax {
void GreenBuilder::StartNode(const SyntaxKind kind) noexcept {
  const std::pair<SyntaxKind, size_t> key =
      std::make_pair(kind, children_.size());

  parents_.emplace_back(key);
  children_.clear();
}

void GreenBuilder::FinishNode() {
  if (parents_.empty()) {
    throw std::invalid_argument("nodes list was empty");
  }

  const auto [kind, first_child] = parents_.back();
  parents_.pop_back();

  const CachedGreenElement entry = cache_.GetNode(kind, children_, first_child);
  children_.emplace_back(entry);
}

inline GreenBuilder::Checkpoint GreenBuilder::CreateCheckpoint()
    const noexcept {
  return {children_.size()};
}

void GreenBuilder::ApplyCheckpoint(const Checkpoint &checkpoint,
                                   const SyntaxKind kind) {
  if (checkpoint.index > children_.size()) {
    throw std::invalid_argument("checkpoint no longer valid");
  }

  if (!parents_.empty()) {
    const auto [_, first_child] = parents_.back();
    if (checkpoint.index < first_child) {
      throw std::invalid_argument("checkpoint no longer valid");
    }
  }

  parents_.emplace_back(kind, checkpoint.index);
}

void GreenBuilder::Token(const SyntaxKind kind,
                         const std::u32string &source) noexcept {
  const CachedGreenElement token = cache_.GetToken(kind, source);
  children_.emplace_back(token);
}
  
GreenNode GreenBuilder::Finish() {
  if (!parents_.empty()) {
    throw std::invalid_argument("unexpected empty stack");
  }

  const auto [_, element] = children_.back();
  children_.pop_back();

  if (const std::optional<GreenNode> node = element.TryGetNode();
      node.has_value()) {
    return node.value();
  } else {
    throw std::invalid_argument("unexpected node");
  }
}

}  // namespace orion::syntax