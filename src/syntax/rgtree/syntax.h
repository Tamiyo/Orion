#ifndef SYNTAX_RGTREE_SYNTAX_H_
#define SYNTAX_RGTREE_SYNTAX_H_

#include <cstddef>
#include <cstdint>
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
  explicit SyntaxNodeData(const size_t offset,
                          std::optional<SyntaxNode<SyntaxKind>> parent,
                          GreenNode<SyntaxKind> green)
      : offset_(offset), parent_(std::move(parent)), green_(std::move(green)) {}

  SyntaxNodeData() = delete;
  SyntaxNodeData(const SyntaxNodeData<SyntaxKind>&) = default;
  SyntaxNodeData(SyntaxNodeData<SyntaxKind>&&) = default;

  [[nodiscard]] size_t Offset() const { return offset_; }
  [[nodiscard]] const std::optional<SyntaxNode<SyntaxKind>>& Parent() const {
    return parent_;
  }
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
  static SyntaxNode<SyntaxKind> CreateRoot(const GreenNode<SyntaxKind>& node) {
    return SyntaxNode<SyntaxKind>(0, node);
  }

  explicit SyntaxNode(size_t offset, SyntaxNode<SyntaxKind> parent,
                      GreenNode<SyntaxKind> green)
      : data_(std::make_shared<SyntaxNodeData<SyntaxKind>>(
            offset, std::make_optional(std::move(parent)), std::move(green))) {}

  explicit SyntaxNode(size_t offset, GreenNode<SyntaxKind> green)
      : data_(std::make_shared<SyntaxNodeData<SyntaxKind>>(offset, std::nullopt,
                                                           std::move(green))) {}

  SyntaxNode() = delete;

  [[nodiscard]] size_t Offset() const noexcept { return data_->Offset(); }

  [[nodiscard]] const std::optional<SyntaxNode<SyntaxKind>>& Parent()
      const noexcept {
    return data_->Parent();
  }

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
  explicit SyntaxTokenData(const size_t offset,
                           std::optional<SyntaxNode<SyntaxKind>> parent,
                           GreenToken<SyntaxKind> green)
      : offset_(offset), parent_(std::move(parent)), green_(std::move(green)) {}


  SyntaxTokenData() = delete;
  SyntaxTokenData(const SyntaxTokenData<SyntaxKind>&) = default;
  SyntaxTokenData(SyntaxTokenData<SyntaxKind>&&) = default;


  [[nodiscard]] size_t Offset() const { return offset_; }

  [[nodiscard]] const std::optional<SyntaxNode<SyntaxKind>>& Parent() const {
    return parent_;
  }

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
  explicit SyntaxToken(size_t offset, const SyntaxNode<SyntaxKind>& parent,
                       const GreenToken<SyntaxKind>& green)
      : data_(std::make_shared<SyntaxTokenData<SyntaxKind>>(
            offset, std::make_optional(parent), green)) {}

  explicit SyntaxToken(size_t offset, const GreenToken<SyntaxKind>& green)
      : data_(std::make_shared<SyntaxTokenData<SyntaxKind>>(
            offset, std::nullopt, green)) {}

  SyntaxToken() = delete;


  [[nodiscard]] size_t Offset() const noexcept { return data_->Offset(); }


  [[nodiscard]] const std::optional<SyntaxNode<SyntaxKind>>& Parent()
      const noexcept {
    return data_->Parent();
  }

 
  [[nodiscard]] const GreenToken<SyntaxKind>& Green() const noexcept {
    return data_->Green();
  }

  [[nodiscard]] SyntaxKind Kind() const noexcept {
    return data_->Green().Kind();
  }

 private:
  const std::shared_ptr<SyntaxTokenData<SyntaxKind>> data_;
};

template <typename SyntaxKind = uint16_t>
class SyntaxElement {
 private:
  using GreenElement = GreenElement<SyntaxKind>;
  using GreenNode = GreenNode<SyntaxKind>;
  using GreenToken = GreenToken<SyntaxKind>;
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
    }

    const SyntaxToken& token = std::get<SyntaxToken>(variant_);
    return token.Kind();
  }

  [[nodiscard]] SyntaxNode Parent() const noexcept {
    if (std::holds_alternative<SyntaxNode>(variant_)) {
      const SyntaxNode& node = std::get<SyntaxNode>(variant_);
      return node.Parent();
    } else {
      const SyntaxToken& token = std::get<SyntaxToken>(variant_);
      return token.Parent();
    }
  }

 private:
  const std::variant<SyntaxNode, SyntaxToken> variant_;
};
}  // namespace yuzu::syntax

#endif  // SYNTAX_RGTREE_SYNTAX_H_
