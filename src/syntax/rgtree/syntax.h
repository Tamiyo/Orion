#ifndef SYNTAX_RGTREE_SYNTAX_H_
#define SYNTAX_RGTREE_SYNTAX_H_

#include <cstddef>
#include <memory>
#include <optional>
#include <utility>

#include "syntax/rgtree/green.h"

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
  SyntaxNodeData(SyntaxNodeData<SyntaxKind>&&) = default;

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
  const size_t offset_;
  const std::optional<SyntaxNode<SyntaxKind>> parent_;
  const GreenNode<SyntaxKind> green_;
};

/// \brief Represents a syntax node in the syntax tree.
///
/// `SyntaxNode` encapsulates `SyntaxNodeData` and provides access to
/// the node's properties and methods for interacting with the syntax tree.
template <typename SyntaxKind>
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
  [[nodiscard]] size_t Offset() const noexcept { return data_->Offset(); }

  /// \brief Returns the optional parent syntax node.
  ///
  /// \return A reference to the optional parent `SyntaxNode`.
  [[nodiscard]] const std::optional<SyntaxNode<SyntaxKind>>& Parent()
      const noexcept {
    return data_->Parent();
  }

  /// \brief Returns the associated green node.
  ///
  /// \return A reference to the `GreenNode`.
  [[nodiscard]] const GreenNode<SyntaxKind>& Green() const noexcept {
    return data_->Green();
  }

  [[nodiscard]] SyntaxKind Kind() const noexcept {
    return data_->Green().Kind();
  }

 private:
  const std::shared_ptr<SyntaxNodeData<SyntaxKind>> data_;
};

/// \brief Represents the data associated with a syntax token.
///
/// `SyntaxTokenData` holds the offset of the token, a pointer to its parent
/// syntax node, and the associated green token.
template <typename SyntaxKind = uint16_t>
class SyntaxTokenData {
 public:
  /// \brief Constructs a `SyntaxTokenData` with the specified offset, parent
  /// node, and green token.
  ///
  /// \param offset The offset of the token in the source.
  /// \param parent Pointer to the parent `SyntaxNode`.
  /// \param green The associated `GreenToken`.
  explicit SyntaxTokenData(const size_t offset,
                           std::optional<SyntaxNode<SyntaxKind>> parent,
                           GreenToken<SyntaxKind> green)
      : offset_(offset), parent_(std::move(parent)), green_(std::move(green)) {}

  /// \brief Deleted default constructor.
  ///
  /// A `SyntaxTokenData` must always be constructed with an offset and a green
  /// token.
  SyntaxTokenData() = delete;

  /// Defaulted copy and move constructors.
  SyntaxTokenData(const SyntaxTokenData<SyntaxKind>&) = default;
  SyntaxTokenData(SyntaxTokenData<SyntaxKind>&&) = default;

  /// \brief Returns the offset of the token.
  ///
  /// \return The token's offset in the source.
  [[nodiscard]] size_t Offset() const { return offset_; }

  /// \brief Returns the optional parent syntax node.
  ///
  /// \return A reference to the optional parent `SyntaxNode`.
  [[nodiscard]] const std::optional<SyntaxNode<SyntaxKind>>& Parent() const {
    return parent_;
  }

  /// \brief Returns the associated green token.
  ///
  /// \return A reference to the `GreenToken`.
  [[nodiscard]] const GreenToken<SyntaxKind>& Green() const { return green_; }

 private:
  const size_t offset_;
  const std::optional<SyntaxNode<SyntaxKind>> parent_;
  const GreenToken<SyntaxKind> green_;
};

/// \brief Represents a syntax token in the syntax tree.
///
/// `SyntaxToken` encapsulates `SyntaxTokenData` and provides access to
/// the token's properties and methods for interacting with the syntax tree.
template <typename SyntaxKind = uint16_t>
class SyntaxToken {
 public:
  /// \brief Constructs a `SyntaxToken` with the specified offset, parent node,
  /// and green token.
  ///
  /// \param offset The offset of the token in the source.
  /// \param parent Pointer to the parent `SyntaxNode`.
  /// \param green The associated `GreenToken`.
  explicit SyntaxToken(size_t offset, const SyntaxNode<SyntaxKind>& parent,
                       const GreenToken<SyntaxKind>& green)
      : data_(std::make_shared<SyntaxTokenData<SyntaxKind>>(
            offset, std::make_optional(parent), green)) {}

  /// \brief Constructs a `SyntaxToken` with the specified offset and green
  /// token, with no parent.
  ///
  /// \param offset The offset of the token in the source.
  /// \param green The associated `GreenToken`.
  explicit SyntaxToken(size_t offset, const GreenToken<SyntaxKind>& green)
      : data_(std::make_shared<SyntaxTokenData<SyntaxKind>>(
            offset, std::nullopt, green)) {}

  /// \brief Deleted default constructor.
  ///
  /// A `SyntaxToken` must always be constructed with an offset and a green
  /// token.
  SyntaxToken() = delete;

  /// \brief Returns the offset of the token.
  ///
  /// \return The token's offset in the source.
  [[nodiscard]] size_t Offset() const noexcept { return data_->Offset(); }

  /// \brief Returns the optional parent syntax node.
  ///
  /// \return A reference to the optional parent `SyntaxNode`.
  [[nodiscard]] const std::optional<SyntaxNode<SyntaxKind>>& Parent()
      const noexcept {
    return data_->Parent();
  }

  /// \brief Returns the associated green token.
  ///
  /// \return A reference to the `GreenToken`.
  [[nodiscard]] const GreenToken<SyntaxKind>& Green() const noexcept {
    return data_->Green();
  }

 private:
  const std::shared_ptr<SyntaxTokenData<SyntaxKind>> data_;
};

template <typename SyntaxKind = uint16_t>
class SyntaxElement {
 private:
  using SyntaxNode = SyntaxNode<SyntaxKind>;
  using SyntaxToken = SyntaxToken<SyntaxKind>;

 public:
  explicit SyntaxElement(SyntaxNode node) : variant_(std::move(node)) {}
  explicit SyntaxElement(SyntaxToken token) : variant_(std::move(token)) {}

  SyntaxElement() = delete;

  [[nodiscard]] SyntaxKind Kind() const noexcept {
    if (std::holds_alternative<SyntaxNode>(variant_)) {
      const SyntaxNode& node = std::get<SyntaxNode>(variant_);
      return node.Kind();
    } else {
      const SyntaxNode& token = std::get<SyntaxToken>(variant_);
      return token.Kind();
    }
  }

  [[nodiscard]] SyntaxNode Parent() const noexcept {
    if (std::holds_alternative<SyntaxNode>(variant_)) {
      const SyntaxNode& node = std::get<SyntaxNode>(variant_);
      return node.Parent();
    } else {
      const SyntaxNode& token = std::get<SyntaxToken>(variant_);
      return token.Parent();
    }
  }

 private:
  const std::variant<SyntaxNode, SyntaxToken> variant_;
};
}  // namespace yuzu::syntax

#endif  // SYNTAX_RGTREE_SYNTAX_H_
