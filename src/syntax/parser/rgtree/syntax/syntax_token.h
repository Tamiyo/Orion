#ifndef SYNTAX_PARSER_RGTREE_SYNTAX_SYNTAX_TOKEN_H_
#define SYNTAX_PARSER_RGTREE_SYNTAX_SYNTAX_TOKEN_H_

#include <memory>
#include <optional>

#include "syntax/parser/rgtree/green/green_token.h"
#include "syntax/parser/rgtree/syntax/syntax_node.h"

namespace orion::syntax {

/**
 * @brief Represents the data associated with a syntax token.
 *
 * `SyntaxTokenData` holds the offset of the token, a pointer to its parent
 * syntax node, and the associated green token.
 */
class SyntaxTokenData {
 public:
  /**
   * @brief Constructs a `SyntaxTokenData` with the specified offset, parent
   * node, and green token.
   *
   * @param offset The offset of the token in the source.
   * @param parent Pointer to the parent `SyntaxNode`.
   * @param green The associated `GreenToken`.
   */
  explicit SyntaxTokenData(const size_t offset, std::optional<SyntaxNode> parent,
                           GreenToken green)
      : offset_(offset), parent_(std::move(parent)), green_(std::move(green)) {}

  /**
   * @brief Deleted default constructor.
   *
   * A `SyntaxTokenData` must always be constructed with an offset and a green
   * token.
   */
  SyntaxTokenData() = delete;

  /** Defaulted copy and move constructors. */
  SyntaxTokenData(const SyntaxTokenData&) = default;
  SyntaxTokenData(SyntaxTokenData&&) = default;

  /**
   * @brief Returns the offset of the token.
   *
   * @return The token's offset in the source.
   */
  [[nodiscard]] size_t Offset() const { return offset_; }

  /**
   * @brief Returns the optional parent syntax node.
   *
   * @return A reference to the optional parent `SyntaxNode`.
   */
  [[nodiscard]] const std::optional<SyntaxNode>& Parent() const {
    return parent_;
  }

  /**
   * @brief Returns the associated green token.
   *
   * @return A reference to the `GreenToken`.
   */
  [[nodiscard]] const GreenToken& Green() const { return green_; }

 private:
  /** The offset of the token in the source. */
  const size_t offset_;

  /** The parent syntax node, if any. */
  const std::optional<SyntaxNode> parent_;

  /** The associated green token. */
  const GreenToken green_;
};

/**
 * @brief Represents a syntax token in the syntax tree.
 *
 * `SyntaxToken` encapsulates `SyntaxTokenData` and provides access to
 * the token's properties and methods for interacting with the syntax tree.
 */
class SyntaxToken {
 public:
  /**
   * @brief Constructs a `SyntaxToken` with the specified offset, parent node,
   * and green token.
   *
   * @param offset The offset of the token in the source.
   * @param parent Pointer to the parent `SyntaxNode`.
   * @param green The associated `GreenToken`.
   */
  explicit SyntaxToken(size_t offset, const SyntaxNode& parent,
                       const GreenToken& green)
      : data_(std::make_shared<SyntaxTokenData>(
            offset, std::make_optional(parent), green)) {}

  /**
   * @brief Constructs a `SyntaxToken` with the specified offset and green
   * token, with no parent.
   *
   * @param offset The offset of the token in the source.
   * @param green The associated `GreenToken`.
   */
  explicit SyntaxToken(size_t offset, const GreenToken& green)
      : data_(std::make_shared<SyntaxTokenData>(offset, std::nullopt, green)) {}

  /**
   * @brief Deleted default constructor.
   *
   * A `SyntaxToken` must always be constructed with an offset and a green
   * token.
   */
  SyntaxToken() = delete;

  /**
   * @brief Returns the offset of the token.
   *
   * @return The token's offset in the source.
   */
  [[nodiscard]] size_t Offset() const noexcept { return data_->Offset(); }

  /**
   * @brief Returns the optional parent syntax node.
   *
   * @return A reference to the optional parent `SyntaxNode`.
   */
  [[nodiscard]] const std::optional<SyntaxNode>& Parent() const noexcept {
    return data_->Parent();
  }

  /**
   * @brief Returns the associated green token.
   *
   * @return A reference to the `GreenToken`.
   */
  [[nodiscard]] const GreenToken& Green() const noexcept {
    return data_->Green();
  }

 private:
  /** Pointer to the token data. */
  const std::shared_ptr<SyntaxTokenData> data_;
};

}  // namespace orion::syntax

#endif  // SYNTAX_PARSER_RGTREE_SYNTAX_SYNTAX_TOKEN_H_
