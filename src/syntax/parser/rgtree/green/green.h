#ifndef SYNTAX_PARSER_RGTREE_GREEN_GREEN_H_
#define SYNTAX_PARSER_RGTREE_GREEN_GREEN_H_

#include <memory>
#include <variant>
#include <vector>
#include <utility>

#include "syntax/syntax_kind.h"

namespace orion::syntax {

class GreenElement;

/// \brief Represents the data associated with a green node.
///
/// `GreenNodeData` holds information about the kind of node, its width,
/// and its child elements. This structure is used during parsing and
/// tree construction.
class GreenNodeData {
 public:
  /// \brief Constructs a `GreenNodeData` with the specified token kind, width,
  /// and child elements.
  ///
  /// \param kind The type of the node as defined by `SyntaxKind`.
  /// \param width The width of the node in terms of layout.
  /// \param children The child elements contained within this node.
  explicit GreenNodeData(const SyntaxKind kind, const size_t width,
                         std::vector<GreenElement> children);

  /// \brief Deleted default constructor.
  ///
  /// A `GreenNodeData` must always be constructed with a kind, width, and
  /// children.
  GreenNodeData() = delete;

  /// Defaulted copy and move constructors.
  GreenNodeData(const GreenNodeData&) = default;
  GreenNodeData(GreenNodeData&&) = default;

  /// \brief Returns the kind of the node.
  ///
  /// \return The node's `SyntaxKind`.
  [[nodiscard]] SyntaxKind Kind() const { return kind_; }

  /// \brief Returns the width of the node.
  ///
  /// \return The width of the node.
  [[nodiscard]] size_t Width() const { return width_; }

  /// \brief Returns the child elements of the node.
  ///
  /// \return A reference to the vector of child `GreenElement`s.
  [[nodiscard]] const std::vector<GreenElement>& Children() const {
    return children_;
  }

  /// \brief Compares two `GreenNodeData` objects for equality.
  ///
  /// \param other The other `GreenNodeData` to compare with.
  /// \return `true` if both nodes have the same kind, width, and children,
  /// otherwise `false`.
  bool operator==(const GreenNodeData& other) const;

 private:
  /// The type of the node.
  const SyntaxKind kind_;

  /// The width of the node.
  const size_t width_;

  /// The child elements of the node.
  const std::vector<GreenElement> children_;
};

/// \brief Represents a green node in the syntax tree.
///
/// `GreenNode` encapsulates `GreenNodeData` and provides access to node
/// properties and methods for interacting with the node's children.
class GreenNode {
 public:
  /// \brief Constructs a `GreenNode` with the specified kind and child
  /// elements.
  ///
  /// \param kind The type of the node as defined by `SyntaxKind`.
  /// \param children The child elements contained within this node.
  explicit GreenNode(const SyntaxKind kind,
                     const std::vector<GreenElement>& children);

  /// \brief Deleted default constructor.
  ///
  /// A `GreenNode` must always be constructed with a kind and children.
  GreenNode() = delete;

  /// Defaulted copy and move constructors.
  GreenNode(const GreenNode&) = default;
  GreenNode(GreenNode&&) noexcept = default;

  /// \brief Returns the kind of the node.
  ///
  /// \return The node's `SyntaxKind`.
  [[nodiscard]] SyntaxKind Kind() const { return data_->Kind(); }

  /// \brief Returns the width of the node.
  ///
  /// \return The width of the node.
  [[nodiscard]] size_t Width() const { return data_->Width(); }

  /// \brief Returns the child elements of the node.
  ///
  /// \return A reference to the vector of child `GreenElement`s.
  [[nodiscard]] const std::vector<GreenElement>& Children() const {
    return data_->Children();
  }

  /// \brief Returns the current use count of the shared node data.
  ///
  /// \return The number of `GreenNode` instances sharing the same
  /// `GreenNodeData`.
  [[nodiscard]] size_t UseCount() const { return data_.use_count(); }

  /// \brief Compares two `GreenNode` objects for equality.
  ///
  /// \param other The other `GreenNode` to compare with.
  /// \return `true` if both nodes share the same underlying data, otherwise
  /// `false`.
  bool operator==(const GreenNode& other) const { return data_ == other.data_; }

 private:
  /// \brief Computes the width of the node based on its children.
  ///
  /// \param children The child elements to compute the width for.
  /// \return The computed width of the node.
  [[nodiscard]] static size_t ComputeWidth(
      const std::vector<GreenElement>& children);

  /// Shared data for the node.
  const std::shared_ptr<GreenNodeData> data_;
};

/// \brief Represents the data associated with a green token.
///
/// `GreenTokenData` holds the kind of token and its source text, which
/// are used during parsing and syntax tree construction.
class GreenTokenData {
 public:
  /// \brief Constructs a `GreenTokenData` with the specified token kind and
  /// source text.
  ///
  /// \param kind The type of the token as defined by `SyntaxKind`.
  /// \param source The actual text content of the token.
  explicit GreenTokenData(const SyntaxKind kind, std::u32string_view source)
      : kind_(kind), source_(std::move(source)) {}

  /// \brief Deleted default constructor.
  ///
  /// A `GreenTokenData` must always be constructed with a kind and source.
  GreenTokenData() = delete;

  /// Defaulted copy and move constructors.
  GreenTokenData(const GreenTokenData&) = default;
  GreenTokenData(GreenTokenData&&) = default;

  /// \brief Returns the kind of the token.
  ///
  /// \return The token's `SyntaxKind`.
  [[nodiscard]] SyntaxKind Kind() const { return kind_; }

  /// \brief Returns the source text of the token.
  ///
  /// \return A reference to the token's source string.
  [[nodiscard]] const std::u32string_view& Source() const { return source_; }

  /// \brief Compares two `GreenTokenData` objects for equality.
  ///
  /// \param other The other `GreenTokenData` to compare with.
  /// \return `true` if both tokens have the same kind and source text,
  /// otherwise `false`.
  bool operator==(const GreenTokenData& other) const {
    return kind_ == other.kind_ && source_ == other.source_;
  }

 private:
  /// The type of the token.
  const SyntaxKind kind_;

  /// The actual text content of the token.
  const std::u32string_view source_;
};

/// \brief Represents a green token, which encapsulates `GreenTokenData`.
///
/// `GreenToken` uses shared ownership to manage the underlying
/// `GreenTokenData`.
class GreenToken {
 public:
  /// \brief Constructs a `GreenToken` with the specified kind and source text.
  ///
  /// \param kind The type of the token as defined by `SyntaxKind`.
  /// \param source The actual text content of the token.
  explicit GreenToken(const SyntaxKind kind, std::u32string_view source)
      : data_(std::make_shared<GreenTokenData>(GreenTokenData(kind, source))) {}

  /// \brief Deleted default constructor.
  ///
  /// A `GreenToken` must always be constructed with a kind and source.
  GreenToken() = delete;

  /// Defaulted copy and move constructors.
  GreenToken(const GreenToken&) = default;
  GreenToken(GreenToken&&) = default;

  /// \brief Returns the kind of the token.
  ///
  /// \return The token's `SyntaxKind`.
  [[nodiscard]] SyntaxKind Kind() const { return data_->Kind(); }

  /// \brief Returns the source text of the token.
  ///
  /// \return A reference to the token's source string.
  [[nodiscard]] std::u32string_view Source() const { return data_->Source(); }

  /// \brief Returns the current use count of the shared token data.
  ///
  /// \return The number of `GreenToken` instances sharing the same
  /// `GreenTokenData`.
  [[nodiscard]] size_t UseCount() const { return data_.use_count(); }

  /// \brief Compares two `GreenToken` objects for equality.
  ///
  /// \param other The other `GreenToken` to compare with.
  /// \return `true` if both tokens share the same underlying data, otherwise
  /// `false`.
  bool operator==(const GreenToken& other) const {
    return data_ == other.data_;
  }

 private:
  /// Shared data for the token.
  const std::shared_ptr<GreenTokenData> data_;
};

/// \brief Represents a green element, which can be either a `GreenNode` or a
/// `GreenToken`.
///
/// The `GreenElement` class uses a `std::variant` to hold either a node or a
/// token, allowing for flexible representation of syntax tree elements.
class GreenElement {
 public:
  /// \brief Constructs a `GreenElement` from a `GreenNode`.
  ///
  /// \param node The `GreenNode` to be stored in the element.
  explicit GreenElement(GreenNode node) : variant_(std::move(node)) {}

  /// \brief Constructs a `GreenElement` from a `GreenToken`.
  ///
  /// \param token The `GreenToken` to be stored in the element.
  explicit GreenElement(GreenToken token) : variant_(std::move(token)) {}

  /// \brief Default constructor that initializes the element to an empty state.
  explicit GreenElement() : variant_(std::monostate()) {}

  /// Defaulted copy and move constructors.
  GreenElement(const GreenElement&) = default;
  GreenElement(GreenElement&&) = default;

  /// \brief Move assignment operator.
  ///
  /// \param other The `GreenElement` to move from.
  /// \return A reference to this `GreenElement`.
  GreenElement& operator=(GreenElement&& other) noexcept { return other; }

  /// \brief Checks if the element holds a `GreenNode`.
  ///
  /// \return `true` if the element is a `GreenNode`, otherwise `false`.
  [[nodiscard]] bool IsNode() const noexcept {
    return std::holds_alternative<GreenNode>(variant_);
  }

  /// \brief Checks if the element holds a `GreenToken`.
  ///
  /// \return `true` if the element is a `GreenToken`, otherwise `false`.
  [[nodiscard]] bool IsToken() const noexcept {
    return std::holds_alternative<GreenToken>(variant_);
  }

  /// \brief Attempts to retrieve the stored `GreenNode`.
  ///
  /// \return An optional containing the `GreenNode` if it is present, otherwise
  /// `nullopt`.
  [[nodiscard]] std::optional<GreenNode> TryGetNode() const noexcept {
    if (std::holds_alternative<GreenNode>(variant_)) {
      return std::make_optional(std::get<GreenNode>(variant_));
    }

    return std::nullopt;
  }

  /// \brief Attempts to retrieve the stored `GreenToken`.
  ///
  /// \return An optional containing the `GreenToken` if it is present,
  /// otherwise `nullopt`.
  [[nodiscard]] std::optional<GreenToken> TryGetToken() const noexcept {
    if (std::holds_alternative<GreenToken>(variant_)) {
      return std::make_optional(std::get<GreenToken>(variant_));
    }

    return std::nullopt;
  }

  /// \brief Returns the current use count of the stored element's data.
  ///
  /// \return The number of instances sharing the same `GreenNode` or
  /// `GreenToken`.
  [[nodiscard]] size_t UseCount() const noexcept;

  /// \brief Compares two `GreenElement` objects for equality.
  ///
  /// \param other The other `GreenElement` to compare with.
  /// \return `true` if both elements are equal, otherwise `false`.
  bool operator==(const GreenElement& other) const noexcept {
    return variant_ == other.variant_;
  }

 private:
  /// Variant that can hold either a `GreenNode`, `GreenToken`, or a
  /// `monostate`.
  const std::variant<GreenNode, GreenToken, std::monostate> variant_;
};
}  // namespace orion::syntax

#endif  // SYNTAX_PARSER_RGTREE_GREEN_GREEN_H_
