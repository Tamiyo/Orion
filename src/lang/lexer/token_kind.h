#ifndef SYNTAX_LEXER_TOKEN_KIND_H_
#define SYNTAX_LEXER_TOKEN_KIND_H_

#include <cstdint>
#include <sstream>
#include <string_view>

namespace yuzu::lang {

/// \brief Represents the different kinds of tokens in the lexer.
///
/// This enum class categorizes various token types encountered during
/// lexical analysis, including literals, operators, punctuation, and keywords.
enum class TokenKind : uint16_t {
  // --- Trivia ---
  /// Whitespace (e.g. spaces, tabs).
  kWhitespace,
  /// Newline characters.
  kNewline,
  /// Line or block comments.
  kComment,

  // --- Keywords ---
  // (To be added.)

  // --- Punctuation ---
  /// Dot or period (e.g. `.`).
  kDot,

  /// Plus sign (e.g. `+`).
  kPlus,
  /// Minus sign (e.g. `-`).
  kMinus,
  /// Asterisk or multiplication sign (e.g. `*`).
  kAsterisk,
  /// Slash or division operator (e.g. `/`).
  kSlash,
  /// Percent or modulo operator (e.g. `%`).
  kPercent,

  // --- Boolean Literals ---
  /// Boolean literal (`true`, `false`).
  kBooleanLiteral,

  // --- String Literals ---
  /// A string literal (e.g. `"hello"`).
  kStringLiteral,

  // --- Exact Numeric Literals ---
  /// An integer literal.
  kIntLiteral,
  /// A big integer literal (platform-dependent).
  kBigIntLiteral,
  /// A small integer literal.
  kSmallIntLiteral,
  /// A tiny integer literal.
  kTinyIntLiteral,

  // --- Approx Numeric Literals ---
  /// A floating-point number literal.
  kFloatLiteral,
  /// A double precision float literal.
  kDoubleLit,
  /// A high-precision decimal literal.
  kBigDecimalLiteral,

  // --- Other ---
  /// An unquoted identifier (e.g. variable name).
  kIdentifier,
  /// A quoted identifier (e.g. `"column"`).
  kQuotedIdentifier,

  // --- Special ---
  /// End-of-file marker.
  kEof,
};

/// \brief Checks whether a token kind is considered trivia (e.g., whitespace or
/// comment).
///
/// \param kind The token kind to check.
/// \return `true` if the token is trivia; otherwise `false`.
constexpr bool IsTrivia(const TokenKind kind) noexcept {
  switch (kind) {
    case TokenKind::kWhitespace:
    case TokenKind::kNewline:
    case TokenKind::kComment:
      return true;
    default:
      return false;
  }
};
}  // namespace yuzu::syntax

#endif  // SYNTAX_LEXER_TOKEN_KIND_H_
