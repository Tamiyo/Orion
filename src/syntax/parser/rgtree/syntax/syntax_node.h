#ifndef SYNTAX_PARSER_RGTREE_SYNTAX_SYNTAX_NODE_H_
#define SYNTAX_PARSER_RGTREE_SYNTAX_SYNTAX_NODE_H_

#include <cstddef>
#include <memory>
#include <optional>
#include <utility>

#include "syntax/parser/rgtree/green/green.h"

namespace yuzu::syntax {

/// Forward declare SyntaxNode for `SyntaxNodeData`.
template <typename SyntaxKind = uint16_t>
class SyntaxNode;

/// \brief Represents the data associated with a syntax node.
///
/// `SyntaxNodeData` holds the offset of the node, a pointer to its parent
/// syntax node, and the associated green node.
template <typename SyntaxKind = uint16_t>
class SyntaxNodeData {
 public:
  /// \brief Constructs a `SyntaxNodeData` with the specified offset, parent
  /// node, and green node.
  ///
  /// \param offset The offset of the node in the source.
  /// \param parent Pointer to the parent `SyntaxNode`.
  /// \param green The associated `GreenNode`.
  explicit SyntaxNodeData(const size_t offset,
                          std::optional<SyntaxNode<SyntaxKind>> parent,
                          GreenNode<SyntaxKind> green)
      : offset_(offset), parent_(std::move(parent)), green_(std::move(green)) {}

  /// \brief Deleted default constructor.
  ///
  /// A `SyntaxNodeData` must always be constructed with an offset and a green
  /// node.
  SyntaxNodeData() = delete;

  /// Defaulted copy and move constructors.
  SyntaxNodeData(const SyntaxNodeData<SyntaxKind>&) = default;
  SyntaxNodeData(SyntaxNodeDat<SyntaxKind> a&&) = default;

  /// \brief Returns the offset of the node.
  ///
  /// \return The node's offset in the source.
  [[nodiscard]] size_t Offset() const { return offset_; }

  /// \brief Returns the optional parent syntax node.
  ///
  /// \return A reference to the optional parent `SyntaxNode`.
  [[nodiscard]] const std::optional<SyntaxNode<SyntaxKind>>& Parent() const {
    return parent_;
  }

  /// \brief Returns the associated green node.
  ///
  /// \return A reference to the `GreenNode`.
  [[nodiscard]] const GreenNode<SyntaxKind>& Green() const { return green_; }

 private:
  /// The offset of the node in the source.
  const size_t offset_;

  /// The parent syntax node, if any.
  const std::optional<SyntaxNode<SyntaxKind>> parent_;

  /// The associated green node.
  const GreenNode<SyntaxKind> green_;
};

/// \brief Represents a syntax node in the syntax tree.
///
/// `SyntaxNode` encapsulates `SyntaxNodeData` and provides access to
/// the node's properties and methods for interacting with the syntax tree.
template <typename SyntaxKind = uint16_t>
class SyntaxNode {
 public:
  /// \brief Creates a root syntax node from a green node.
  ///
  /// \param node The associated `GreenNode`.
  /// \return A new `SyntaxNode` representing the root.
  static SyntaxNode<SyntaxKind> CreateRoot(const GreenNode<SyntaxKind>& node) {
    return SyntaxNode<SyntaxKind>(0, node);
  }

  /// \brief Constructs a `SyntaxNode` with the specified offset, parent node,
  /// and green node.
  ///
  /// \param offset The offset of the node in the source.
  /// \param parent Pointer to the parent `SyntaxNode`.
  /// \param green The associated `GreenNode`.
  explicit SyntaxNode(size_t offset, SyntaxNode<SyntaxKind> parent,
                      GreenNode<SyntaxKind> green)
      : data_(std::make_shared<SyntaxNodeData<SyntaxKind>>(
            offset, std::make_optional(std::move(parent)), std::move(green))) {}

  /// \brief Constructs a `SyntaxNode` with the specified offset and green node,
  /// with no parent.
  ///
  /// \param offset The offset of the node in the source.
  /// \param green The associated `GreenNode`.
  explicit SyntaxNode(size_t offset, GreenNode<SyntaxKind> green)
      : data_(std::make_shared<SyntaxNodeData<SyntaxKind>>(offset, std::nullopt,
                                                           std::move(green))) {}

  /// \brief Deleted default constructor.
  ///
  /// A `SyntaxNode` must always be constructed with an offset and a green node.
  SyntaxNode() = delete;

  /// \brief Returns the offset of the node.
  ///
  /// \return The node's offset in the source.
  [[nodiscard]] size_t GetOffset() const noexcept { return data_->Offset(); }

  /// \brief Returns the optional parent syntax node.
  ///
  /// \return A reference to the optional parent `SyntaxNode`.
  [[nodiscard]] const std::optional<SyntaxNode<SyntaxKind>>& GetParent()
      const noexcept {
    return data_->Parent();
  }

  /// \brief Returns the associated green node.
  ///
  /// \return A reference to the `GreenNode`.
  [[nodiscard]] const GreenNode<SyntaxKind>& GetGreen() const noexcept {
    return data_->Green();
  }

 private:
  /// Pointer to the node data.
  const std::shared_ptr<SyntaxNodeData<SyntaxKind>> data_;
};

}  // namespace yuzu::syntax

#endif  // SYNTAX_PARSER_RGTREE_SYNTAX_SYNTAX_NODE_H_
