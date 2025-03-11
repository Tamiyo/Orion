#ifndef SYNTAX_PARSER_RGTREE_GREEN_GREEN_BUILDER_H_
#define SYNTAX_PARSER_RGTREE_GREEN_GREEN_BUILDER_H_

#include <string>
#include <vector>

#include "syntax/parser/rgtree/green/green_cache.h"
#include "syntax/parser/rgtree/green/green_element.h"
#include "syntax/parser/syntax_kind.h"

namespace orion::syntax {

/** Maximum number of child nodes a green node can have. */
constexpr size_t kMaxNodeSize = 3;

/**
 * @brief Constructs and manages green nodes in the syntax tree.
 *
 * The `GreenBuilder` class provides methods for starting and finishing nodes,
 * managing checkpoints, and adding tokens to the syntax tree structure.
 */
class GreenBuilder {
 public:
  /**
   * @brief Represents a checkpoint in the green builder's state.
   *
   * The `Checkpoint` struct allows for restoring the builder's state to a
   * previous point during node construction.
   */
  struct Checkpoint {
    /** The index of the checkpoint in the children vector. */
    const size_t index;
  };

  /**
   * @brief Constructs a `GreenBuilder`.
   *
   * Initializes the builder with a cache for reusing green elements.
   */
  explicit GreenBuilder() : cache_(GreenCache(kMaxNodeSize)) {}

  /**
   * @brief Starts a new node of the specified kind.
   *
   * @param kind The type of node to start as defined by `SyntaxKind`.
   */
  void StartNode(SyntaxKind kind) noexcept;

  /**
   * @brief Finishes the current node construction.
   *
   * This method finalizes the node and adds it to the children.
   */
  void FinishNode();

  /**
   * @brief Creates a checkpoint of the current state.
   *
   * @return A `Checkpoint` representing the current state of the builder.
   */
  [[nodiscard]] Checkpoint CreateCheckpoint() const noexcept;

  /**
   * @brief Applies a previously created checkpoint.
   *
   * Restores the builder's state to the specified checkpoint and starts a new
   * node.
   *
   * @param checkpoint The checkpoint to apply.
   * @param kind The type of node to start after applying the checkpoint.
   */
  void ApplyCheckpoint(const Checkpoint& checkpoint, SyntaxKind kind);

  /**
   * @brief Adds a token to the current node.
   *
   * @param kind The kind of the token as defined by `SyntaxKind`.
   * @param source The source text of the token.
   */
  void Token(SyntaxKind kind, const std::u32string& source) noexcept;

  /**
   * @brief Finalizes the builder and returns the constructed green node.
   *
   * @return The constructed `GreenNode`.
   */
  [[nodiscard]] GreenNode Finish();

  /**
   * @brief Returns the number of parent nodes currently being constructed.
   *
   * @return The size of the parents vector.
   */
  [[nodiscard]] size_t ParentsSize() const noexcept { return parents_.size(); }

  /**
   * @brief Returns the number of child elements for the current node.
   *
   * @return The size of the children vector.
   */
  [[nodiscard]] size_t ChildrenSize() const noexcept {
    return children_.size();
  }

 private:
  /** Vector holding pairs of SyntaxKind and the index of the first child. */
  std::vector<std::pair<SyntaxKind, size_t>> parents_;

  /** Vector holding cached green elements as children. */
  std::vector<CachedGreenElement> children_;

  /** Cache for reusing green elements. */
  GreenCache cache_;
};

}  // namespace orion::syntax

#endif  // SYNTAX_PARSER_RGTREE_GREEN_GREEN_BUILDER_H_
