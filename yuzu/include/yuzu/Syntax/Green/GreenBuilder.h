#ifndef YUZU_SYNTAX_GREEN_GREEN_BUILDER_H
#define YUZU_SYNTAX_GREEN_GREEN_BUILDER_H

#include "yuzu/Syntax/Green/GreenCache.h"
#include "yuzu/Syntax/SyntaxKind.h"

#include <cstddef>
#include <string_view>
#include <vector>

namespace yuzu::syntax {
/// \brief A checkpoint in the GreenBuilder's construction process.
///
/// GreenBuilderCheckpoint represents a saved position in the builder's
/// children vector, allowing nodes to be started at previous positions
/// for complex parsing scenarios like handling operator precedence or
/// recovering from parsing errors.
struct [[nodiscard]] GreenBuilderCheckpoint {
  /// The index in the children vector where this checkpoint was created.
  const size_t index;
};

/// \brief Incremental builder for constructing immutable green syntax trees.
///
/// GreenBuilder provides an efficient way to construct GreenNodes incrementally
/// during parsing. It uses a cache to deduplicate structurally identical nodes
/// and tokens, reducing memory usage. The builder maintains a stack of parent
/// nodes being constructed and a vector of completed children.
///
/// The builder supports starting and finishing nodes hierarchically, adding
/// tokens as leaf elements, creating checkpoints for retroactive node creation,
/// and automatic deduplication through caching.
class [[nodiscard]] GreenBuilder final {
public:
  /// \brief Construct a GreenBuilder with default cache settings.
  ///
  /// Creates a builder with the default maximum cached node size.
  explicit GreenBuilder();

  /// \brief Construct a GreenBuilder with custom cache settings.
  ///
  /// \param maxCachedNodeSize The maximum number of children a node can have
  /// to be eligible for caching. Nodes with more children than this will not
  /// be cached.
  explicit GreenBuilder(const size_t maxCachedNodeSize);

  /// \brief Start constructing a new node.
  ///
  /// Pushes a new parent onto the stack. All subsequent tokens and finished
  /// nodes will become children of this node until finishNode() is called.
  ///
  /// \param kind The syntax kind of the node being started.
  void startNode(const SyntaxKind kind) noexcept;

  /// \brief Finish constructing the current node.
  ///
  /// Pops the most recent parent from the stack, collects all children added
  /// since startNode() was called, creates a GreenNode (potentially from
  /// cache), and adds it as a child element.
  void finishNode() noexcept;

  /// \brief Start constructing a node at a previous checkpoint.
  ///
  /// Allows retroactively wrapping previously added children in a new parent
  /// node. This is useful for handling operator precedence and other parsing
  /// scenarios where the tree structure needs adjustment.
  ///
  /// \param checkpoint The checkpoint marking where the node should start.
  /// \param kind The syntax kind of the node being started.
  void startNodeAt(const GreenBuilderCheckpoint &checkpoint,
                   const SyntaxKind kind) noexcept;

  /// \brief Create a checkpoint at the current position.
  ///
  /// \return A checkpoint that can be used with startNodeAt().
  GreenBuilderCheckpoint checkpoint() const noexcept;

  /// \brief Add a token as a child element.
  ///
  /// Creates a GreenToken (potentially from cache) and adds it to the
  /// children of the current parent node.
  ///
  /// \param kind The syntax kind of the token.
  /// \param source The source text content of the token.
  void token(const SyntaxKind kind, const std::u32string_view &source) noexcept;

  /// \brief Finish building and return the final GreenNode.
  ///
  /// Returns the completed root node. The parents stack must be empty when
  /// this is called, and the last child must be a node (not a token).
  ///
  /// \return The constructed GreenNode.
  GreenNode finish() noexcept;

  /// \brief Get the current size of the parents stack.
  ///
  /// \return The number of unfinished parent nodes.
  [[nodiscard]] size_t getParentsSize() const noexcept {
    return parents.size();
  }

  /// \brief Get the current size of the children vector.
  ///
  /// \return The number of child elements currently stored.
  [[nodiscard]] size_t getChildrenSize() const noexcept {
    return children.size();
  }

private:
  /// \brief Represents a parent node being constructed.
  ///
  /// Tracks the syntax kind and the starting position in the children vector
  /// for a node that has been started but not yet finished.
  struct Parent {
    /// The syntax kind of this parent node.
    const SyntaxKind kind;

    /// The index in the children vector where this parent's children start.
    const size_t firstChild;
  };

  /// Cache for deduplicating structurally identical nodes and tokens.
  GreenCache cache;

  /// Stack of parent nodes currently being constructed.
  std::vector<GreenBuilder::Parent> parents;

  /// Vector of completed child elements.
  std::vector<GreenCacheEntry> children;
};

} // namespace yuzu::syntax

#endif // YUZU_SYNTAX_GREEN_GREEN_BUILDER_H
