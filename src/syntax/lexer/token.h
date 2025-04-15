#ifndef SYNTAX_LEXER_TOKEN_H_
#define SYNTAX_LEXER_TOKEN_H_

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

#include "syntax/lexer/span.h"

namespace yuzu::syntax {

/// \brief Represents a lexical token in the source text.
///
/// A `Token` consists of a kind (denoting its type), a span (indicating its
/// position in the source text), and the actual text content.
template <typename TokenKind = uint16_t>
class Token {
 public:
  /// \brief Constructs a `Token` with a specified kind, span, and source text.
  ///
  /// \param kind The numeric identifier representing the token's type.
  /// \param span The range of text covered by this token in the source input.
  /// \param source The actual text content of the token.
  ///
  /// \note The constructor is explicit to prevent unintended implicit
  /// conversions.
  explicit Token(const TokenKind kind, const Span span,
                 std::u32string_view source)
      : kind_(kind), span_(span), source_(std::move(source)) {}

  /// \brief Deleted default constructor.
  ///
  /// A `Token` must always have a kind, span, and source text, so the default
  /// constructor is deleted.
  Token() = delete;

  /// \brief Returns how long the span (position range) of the token in the
  /// source text is.
  ///
  /// \return The `size_t` representing the length of the token's span.
  [[nodiscard]] size_t Length() const noexcept {
    return span_.End() - span_.Start();
  }

  /// \brief Retrieves the token's kind as a specific enumeration type.
  ///
  /// \tparam TokenKind The enumeration type representing token kinds.
  /// \return The token kind, cast to the specified `TokenKind` type.
  ///
  /// \note This function assumes that `TokenKind` is an enum class where the
  /// token's kind value is a valid enumerator.

  [[nodiscard]] TokenKind Kind() const noexcept {
    return kind_;
  }

  /// \brief Returns the span (position range) of the token in the source text.
  ///
  /// \return The `Span` object representing the start and end positions.
  [[nodiscard]] yuzu::syntax::Span Span() const noexcept { return span_; }

  /// \brief Returns the actual text content of the token.
  ///
  /// \return A reference to the token's source string.
  [[nodiscard]] std::u32string_view Source() const noexcept { return source_; }

  /// \brief Checks if two tokens are equal.
  ///
  /// \param other The token to compare with.
  /// \return `true` if both tokens have the same kind, span, and source text,
  /// otherwise `false`.
  bool operator==(const Token& other) const noexcept {
    return kind_ == other.kind_ && source_ == other.source_ &&
           span_ == other.span_;
  }

 private:
  const TokenKind kind_;
  const yuzu::syntax::Span span_;
  const std::u32string_view source_;
};

}  // namespace yuzu::syntax

#endif  // SYNTAX_LEXER_TOKEN_H_
