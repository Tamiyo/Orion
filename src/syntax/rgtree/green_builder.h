#ifndef SYNTAX_RGTREE_GREEN_BUILDER_H_
#define SYNTAX_RGTREE_GREEN_BUILDER_H_

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include "syntax/rgtree/green.h"
#include "syntax/rgtree/green_cache.h"

namespace yuzu::syntax {
/// Maximum number of child nodes a green node can have.
constexpr size_t kMaxNodeSize = 3;

/// \brief Constructs and manages green nodes in the syntax tree.
///
/// The `GreenBuilder` class provides methods for starting and finishing nodes,
/// managing checkpoints, and adding tokens to the syntax tree structure.
template <typename SyntaxKind = uint16_t>
class GreenBuilder {
 private:
  using GreenCache = GreenCache<SyntaxKind>;
  using GreenNode = GreenNode<SyntaxKind>;

 public:
  /// \brief Represents a checkpoint in the green builder's state.
  ///
  /// The `Checkpoint` struct allows for restoring the builder's state to a
  /// previous point during node construction.
  struct Checkpoint {
    const size_t index;
  };

  explicit GreenBuilder() : cache_(GreenCache(kMaxNodeSize)) {}

  /// \brief Starts a new node of the specified kind.
  void StartNode(const SyntaxKind kind) noexcept {
    const std::pair<SyntaxKind, size_t> key =
        std::make_pair(kind, children_.size());

    parents_.emplace_back(key);
  }

  /// \brief Finishes the current node construction.
  void FinishNode() {
    if (parents_.empty()) {
      throw std::invalid_argument("nodes list was empty");
    }

    const auto [kind, first_child] = parents_.back();
    parents_.pop_back();

    const auto entry = cache_.GetNode(kind, &children_, first_child);

    children_.emplace_back(entry);
  }

  /// \brief Creates a checkpoint of the current state.
  [[nodiscard]] Checkpoint CreateCheckpoint() const noexcept {
    return {children_.size()};
  }

  /// \brief Applies a previously created checkpoint.
  void ApplyCheckpoint(const Checkpoint& checkpoint, const SyntaxKind kind) {
    if (checkpoint.index > children_.size()) {
      throw std::invalid_argument("checkpoint no longer valid");
    }

    if (!parents_.empty()) {
      if (const std::pair<SyntaxKind, size_t> parent = parents_.back();
          checkpoint.index < parent.second) {
        throw std::invalid_argument("checkpoint no longer valid");
      }
    }

    parents_.emplace_back(kind, checkpoint.index);
  }

  /// \brief Adds a token to the current node.
  void Token(const SyntaxKind kind,
             const std::u32string_view& source) noexcept {
    const auto token = cache_.GetToken(kind, source);
    children_.emplace_back(token);
  }

  /// \brief Finalizes the builder and returns the constructed green node.
  [[nodiscard]] GreenNode Finish() {
    if (!parents_.empty()) {
      throw std::invalid_argument("unexpected empty stack");
    }

    const auto entry = children_.back();
    children_.pop_back();

    if (const std::optional<GreenNode> node = entry.Element().TryGetNode();
        node.has_value()) {
      return node.value();
    } else {
      throw std::invalid_argument("unexpected node");
    }
  }

  [[nodiscard]] size_t ParentsSize() const noexcept { return parents_.size(); }

  [[nodiscard]] size_t ChildrenSize() const noexcept {
    return children_.size();
  }

 private:
  std::vector<std::pair<SyntaxKind, size_t>> parents_;
  std::vector<typename GreenCache::Cached> children_;
  GreenCache cache_;
};

}  // namespace yuzu::syntax

#endif  // SYNTAX_RGTREE_GREEN_BUILDER_H_
