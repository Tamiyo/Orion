#ifndef ORION_SYNTAX_LEXER_TOKEN_H_
#define ORION_SYNTAX_LEXER_TOKEN_H_

#include <cstdint>
#include <string>
#include <utility>

#include "syntax/lexer/span.h"

namespace orion::syntax {

/**
 * @brief Represents a lexical token in the source text.
 *
 * A `Token` consists of a kind (denoting its type), a span (indicating its
 * position in the source text), and the actual text content.
 */
class Token {
 public:
  /**
   * @brief Constructs a `Token` with a specified kind, span, and source text.
   *
   * @param kind The numeric identifier representing the token's type.
   * @param span The range of text covered by this token in the source input.
   * @param source The actual text content of the token.
   *
   * @note The constructor is explicit to prevent unintended implicit
   * conversions.
   */
  explicit Token(const uint16_t kind, const Span span, std::u32string source)
      : kind_(kind), span_(span), source_(std::move(source)) {}

  /**
   * @brief Deleted default constructor.
   *
   * A `Token` must always have a kind, span, and source text, so the default
   * constructor is deleted.
   */
  Token() = delete;

  /**
   * @brief Retrieves the token's kind as a specific enumeration type.
   *
   * @tparam TokenKind The enumeration type representing token kinds.
   * @return The token kind, cast to the specified `TokenKind` type.
   *
   * @note This function assumes that `TokenKind` is an enum class where the
   *       token's kind value is a valid enumerator.
   */
  template <typename TokenKind>
  TokenKind GetKind() const {
    return static_cast<TokenKind>(kind_);
  }

  /**
   * @brief Returns the span (position range) of the token in the source text.
   *
   * @return The `Span` object representing the start and end positions.
   */
  [[nodiscard]] orion::syntax::Span Span() const { return span_; }

  /**
   * @brief Returns the actual text content of the token.
   *
   * @return A reference to the token's source string.
   */
  [[nodiscard]] const std::u32string& Source() const { return source_; }

  /**
   * @brief Checks if two tokens are equal.
   *
   * @param other The token to compare with.
   * @return `true` if both tokens have the same kind, span, and source text,
   *         otherwise `false`.
   */
  bool operator==(const Token& other) const {
    return kind_ == other.kind_ && source_ == other.source_ &&
           span_ == other.span_;
  }

 private:
  /** Numeric identifier representing the token's type. */
  const uint16_t kind_;

  /** The span indicating the token's position in the source. */
  const orion::syntax::Span span_;

  /** The actual text content of the token. */
  const std::u32string source_;
};
}  // namespace orion::syntax
#endif  // ORION_SYNTAX_LEXER_TOKEN_H_