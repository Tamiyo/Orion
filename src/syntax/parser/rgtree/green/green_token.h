#ifndef SYNTAX_PARSER_RGTREE_GREEN_GREEN_TOKEN_H_
#define SYNTAX_PARSER_RGTREE_GREEN_GREEN_TOKEN_H_

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "syntax/parser/syntax_kind.h"

namespace orion::syntax {
/**
 * @brief Represents the data associated with a green token.
 *
 * `GreenTokenData` holds the kind of token and its source text, which
 * are used during parsing and syntax tree construction.
 */
class GreenTokenData {
 public:
  /**
   * @brief Constructs a `GreenTokenData` with the specified token kind and
   * source text.
   *
   * @param kind The type of the token as defined by `SyntaxKind`.
   * @param source The actual text content of the token.
   */
  explicit GreenTokenData(const SyntaxKind kind, std::u32string source)
      : kind_(kind), source_(std::move(source)) {}

  /**
   * @brief Deleted default constructor.
   *
   * A `GreenTokenData` must always be constructed with a kind and source.
   */
  GreenTokenData() = delete;

  /** Defaulted copy and move constructors. */
  GreenTokenData(const GreenTokenData&) = default;
  GreenTokenData(GreenTokenData&&) = default;

  /**
   * @brief Returns the kind of the token.
   *
   * @return The token's `SyntaxKind`.
   */
  [[nodiscard]] SyntaxKind Kind() const { return kind_; }

  /**
   * @brief Returns the source text of the token.
   *
   * @return A reference to the token's source string.
   */
  [[nodiscard]] const std::u32string& Source() const { return source_; }

  /**
   * @brief Compares two `GreenTokenData` objects for equality.
   *
   * @param other The other `GreenTokenData` to compare with.
   * @return `true` if both tokens have the same kind and source text, otherwise
   * `false`.
   */
  bool operator==(const GreenTokenData& other) const {
    return kind_ == other.kind_ && source_ == other.source_;
  }

 private:
  /** The type of the token. */
  const SyntaxKind kind_;

  /** The actual text content of the token. */
  const std::u32string source_;
};

/**
 * @brief Represents a green token, which encapsulates `GreenTokenData`.
 *
 * `GreenToken` uses shared ownership to manage the underlying `GreenTokenData`.
 */
class GreenToken {
 public:
  /**
   * @brief Constructs a `GreenToken` with the specified kind and source text.
   *
   * @param kind The type of the token as defined by `SyntaxKind`.
   * @param source The actual text content of the token.
   */
  explicit GreenToken(const SyntaxKind kind, const std::u32string& source)
      : data_(std::make_shared<GreenTokenData>(GreenTokenData(kind, source))) {}

  /**
   * @brief Deleted default constructor.
   *
   * A `GreenToken` must always be constructed with a kind and source.
   */
  GreenToken() = delete;

  /** Defaulted copy and move constructors. */
  GreenToken(const GreenToken&) = default;
  GreenToken(GreenToken&&) = default;

  /**
   * @brief Returns the kind of the token.
   *
   * @return The token's `SyntaxKind`.
   */
  [[nodiscard]] SyntaxKind Kind() const { return data_->Kind(); }

  /**
   * @brief Returns the source text of the token.
   *
   * @return A reference to the token's source string.
   */
  [[nodiscard]] const std::u32string& Source() const { return data_->Source(); }

  /**
   * @brief Returns the current use count of the shared token data.
   *
   * @return The number of `GreenToken` instances sharing the same
   * `GreenTokenData`.
   */
  [[nodiscard]] size_t UseCount() const { return data_.use_count(); }

  /**
   * @brief Compares two `GreenToken` objects for equality.
   *
   * @param other The other `GreenToken` to compare with.
   * @return `true` if both tokens share the same underlying data, otherwise
   * `false`.
   */
  bool operator==(const GreenToken& other) const {
    return data_ == other.data_;
  }

 private:
  /** Shared data for the token. */
  const std::shared_ptr<GreenTokenData> data_;
};
}  // namespace orion::syntax

#endif  // SYNTAX_PARSER_RGTREE_GREEN_GREEN_TOKEN_H_
