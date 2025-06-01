#ifndef SYNTAX_RGTREE_GREEN_H_
#define SYNTAX_RGTREE_GREEN_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <variant>
#include <vector>

namespace yuzu::syntax {
template <typename SyntaxKind = uint16_t>
class GreenElement;

/// \brief Represents the data associated with a green token.
///
/// `GreenTokenData` holds the kind of token and its source text, which
/// are used during parsing and syntax tree construction.
template <typename SyntaxKind = uint16_t>
class GreenTokenData {
 public:
  explicit GreenTokenData(const SyntaxKind kind,
                          const std::u32string_view source)
      : kind_(kind), source_(source) {}

  GreenTokenData() = delete;
  GreenTokenData(const GreenTokenData&) = default;
  GreenTokenData(GreenTokenData&&) = default;

  [[nodiscard]] SyntaxKind Kind() const { return kind_; }

  [[nodiscard]] std::u32string_view Source() const { return source_; }

  [[nodiscard]] bool operator==(const GreenTokenData& other) const {
    return kind_ == other.kind_ && source_ == other.source_;
  }

 private:
  const SyntaxKind kind_;
  const std::u32string_view source_;
};

/// \brief Represents a green token, which encapsulates `GreenTokenData`.
///
/// `GreenToken` uses shared ownership to manage the underlying
/// `GreenTokenData`.
template <typename SyntaxKind = uint16_t>
class GreenToken {
 public:
  explicit GreenToken(const SyntaxKind kind, const std::u32string_view source)
      : data_(std::make_shared<GreenTokenData<SyntaxKind>>(
            GreenTokenData(kind, source))) {}

  GreenToken() = delete;
  GreenToken(const GreenToken&) = default;
  GreenToken(GreenToken&&) = default;

  [[nodiscard]] SyntaxKind Kind() const { return data_->Kind(); }

  [[nodiscard]] std::u32string_view Source() const { return data_->Source(); }

  [[nodiscard]] size_t UseCount() const { return data_.use_count(); }

  bool operator==(const GreenToken& other) const {
    return data_ == other.data_;
  }

 private:
  const std::shared_ptr<GreenTokenData<SyntaxKind>> data_;
};

/// \brief Represents the data associated with a green node.
///
/// `GreenNodeData` holds information about the kind of node, its width,
/// and its child elements. This structure is used during parsing and
/// tree construction.
template <typename SyntaxKind = uint16_t>
class GreenNodeData {
 public:
  explicit GreenNodeData(const SyntaxKind kind, const size_t width,
                         std::vector<GreenElement<SyntaxKind>> children)
      : kind_(kind), width_(width), children_(std::move(children)) {}

  GreenNodeData() = delete;
  GreenNodeData(const GreenNodeData&) = default;
  GreenNodeData(GreenNodeData&&) = default;

  [[nodiscard]] SyntaxKind Kind() const { return kind_; }

  [[nodiscard]] size_t Width() const { return width_; }

  [[nodiscard]] const std::vector<GreenElement<SyntaxKind>>& Children() const {
    return children_;
  }

  bool operator==(const GreenNodeData& other) const {
    return kind_ == other.kind_ && width_ == other.width_ &&
           children_ == other.children_;
  }

 private:
  const SyntaxKind kind_;
  const size_t width_;
  const std::vector<GreenElement<SyntaxKind>> children_;
};

/// \brief Represents a green node in the syntax tree.
///
/// `GreenNode` encapsulates `GreenNodeData` and provides access to node
/// properties and methods for interacting with the node's children.
template <typename SyntaxKind = uint16_t>
class GreenNode {
 public:
  explicit GreenNode(SyntaxKind kind,
                     const std::vector<GreenElement<SyntaxKind>>& children)
      : data_(std::make_shared<GreenNodeData<SyntaxKind>>(
            GreenNodeData<SyntaxKind>(kind, ComputeWidth(children),
                                      children))) {}

  GreenNode() = delete;
  GreenNode(const GreenNode&) = default;
  GreenNode(GreenNode&&) noexcept = default;

  [[nodiscard]] SyntaxKind Kind() const { return data_->Kind(); }

  [[nodiscard]] size_t Width() const { return data_->Width(); }

  [[nodiscard]] const std::vector<GreenElement<SyntaxKind>>& Children() const {
    return data_->Children();
  }

  [[nodiscard]] size_t UseCount() const { return data_.use_count(); }

  bool operator==(const GreenNode& other) const { return data_ == other.data_; }

 private:
  /// \brief Computes the width of the node based on its children.
  ///
  /// \param children The child elements to compute the width for.
  /// \return The computed width of the node.
  [[nodiscard]] static size_t ComputeWidth(
      const std::vector<GreenElement<SyntaxKind>>& children) {
    size_t width = 0;

    for (const GreenElement<SyntaxKind>& child : children) {
      if (const std::optional<GreenNode<SyntaxKind>> node = child.TryGetNode();
          node.has_value()) {
        width += node.value().Width();
        continue;
      }

      if (const std::optional<GreenToken<SyntaxKind>> token =
              child.TryGetToken();
          token.has_value()) {
        width += token.value().Source().size();
        continue;
      }

      throw std::invalid_argument("unknown with object");
    }

    return width;
  }

  const std::shared_ptr<GreenNodeData<SyntaxKind>> data_;
};

/// \brief Represents a green element, which can be either a `GreenNode` or a
/// `GreenToken`.
///
/// The `GreenElement` class uses a `std::variant` to hold either a node or a
/// token, allowing for flexible representation of syntax tree elements.
template <typename SyntaxKind>
class GreenElement {
 public:
  explicit GreenElement(GreenNode<SyntaxKind> node)
      : variant_(std::move(node)) {}

  explicit GreenElement(GreenToken<SyntaxKind> token)
      : variant_(std::move(token)) {}

  explicit GreenElement() : variant_(std::monostate()) {}

  GreenElement(const GreenElement&) = default;
  GreenElement(GreenElement&&) = default;

  GreenElement& operator=(GreenElement&& other) noexcept { return other; }

  [[nodiscard]] bool IsNode() const noexcept {
    return std::holds_alternative<GreenNode<SyntaxKind>>(variant_);
  }

  [[nodiscard]] bool IsToken() const noexcept {
    return std::holds_alternative<GreenToken<SyntaxKind>>(variant_);
  }

  [[nodiscard]] std::optional<GreenNode<SyntaxKind>> TryGetNode()
      const noexcept {
    if (std::holds_alternative<GreenNode<SyntaxKind>>(variant_)) {
      return std::make_optional(std::get<GreenNode<SyntaxKind>>(variant_));
    }

    return std::nullopt;
  }

  [[nodiscard]] std::optional<GreenToken<SyntaxKind>> TryGetToken()
      const noexcept {
    if (std::holds_alternative<GreenToken<SyntaxKind>>(variant_)) {
      return std::make_optional(std::get<GreenToken<SyntaxKind>>(variant_));
    }

    return std::nullopt;
  }

  [[nodiscard]] size_t UseCount() const noexcept {
    if (std::holds_alternative<GreenNode<SyntaxKind>>(variant_)) {
      return std::get<GreenNode<SyntaxKind>>(variant_).UseCount();
    }

    if (std::holds_alternative<GreenToken<SyntaxKind>>(variant_)) {
      return std::get<GreenToken<SyntaxKind>>(variant_).UseCount();
    }

    return 0;  // No shared data for monostate.
  }

  bool operator==(const GreenElement& other) const noexcept {
    return variant_ == other.variant_;
  }

 private:
  const std::variant<GreenNode<SyntaxKind>, GreenToken<SyntaxKind>,
                     std::monostate>
      variant_;
};
}  // namespace yuzu::syntax

#endif  // SYNTAX_RGTREE_GREEN_H_
