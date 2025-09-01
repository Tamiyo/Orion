#include "syntax/green/green_builder.h"

#include <stdexcept>
#include <string>
#include <utility>

#include "syntax/green/green_cache.h"
#include "syntax/syntax_kind.h"

namespace yuzu::syntax {
namespace {
constexpr size_t kMaxNodeSize = 3;
};

GreenBuilder::GreenBuilder() : cache_(GreenCache(kMaxNodeSize)) {}

GreenBuilder::GreenBuilder(const size_t max_node_size)
    : cache_(GreenCache(max_node_size)) {}

void GreenBuilder::StartNode(const SyntaxKind kind) noexcept {
  parents_.emplace_back(GreenBuilder::Parent{kind, children_.size()});
}

void GreenBuilder::FinishNode() {
  if (parents_.empty()) {
    throw std::invalid_argument("nodes list was empty");
  }

  const auto [kind, first_child] = parents_.back();
  parents_.pop_back();

  const auto entry = cache_.GetNode(kind, &children_, first_child);

  children_.emplace_back(entry);
}

void GreenBuilder::StartNodeAt(const GreenBuilderCheckpoint& checkpoint,
                               const SyntaxKind kind) {
  if (checkpoint.index > children_.size()) {
    throw std::invalid_argument("checkpoint no longer valid");
  }

  if (!parents_.empty()) {
    if (const GreenBuilder::Parent parent = parents_.back();
        checkpoint.index < parent.first_child) {
      throw std::invalid_argument("checkpoint no longer valid");
    }
  }

  parents_.emplace_back(GreenBuilder::Parent{kind, checkpoint.index});
}

GreenBuilderCheckpoint GreenBuilder::Checkpoint() const noexcept {
  return {children_.size()};
}

void GreenBuilder::Token(const SyntaxKind kind,
                         const std::u32string& source) noexcept {
  const auto token = cache_.GetToken(kind, source);
  children_.emplace_back(token);
}

GreenNode GreenBuilder::Finish() {
  if (!parents_.empty()) {
    throw std::invalid_argument("unexpected empty stack");
  }

  const auto entry = children_.back();
  children_.pop_back();

  if (const std::optional<GreenNode> node = entry.element.TryGetNode();
      node.has_value()) {
    return node.value();
  } else {
    throw std::invalid_argument("unexpected node");
  }
}

}  // namespace yuzu::syntax
