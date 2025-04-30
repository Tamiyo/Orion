#ifndef LANG_LEXER_TOKEN_KIND_H_
#define LANG_LEXER_TOKEN_KIND_H_

#include <cstdint>

namespace yuzu::lang {

/// \brief Represents the different kinds of tokens in the lexer.
///
/// This enum class categorizes various token types encountered during
/// lexical analysis, including literals, operators, punctuation, and keywords.
enum class TokenKind : uint16_t {
  // --- Trivia ---
  kWhitespace,  /// Whitespace (e.g. spaces, tabs).
  kNewline,     /// Newline characters.
  kComment,     /// Line or block comments.

  // --- Keywords ---
  // (To be added.)

  // --- Punctuation ---
  kDot,          /// Dot or period (e.g. `.`).
  kPlus,         /// Plus sign (e.g. `+`).
  kMinus,        /// Minus sign (e.g. `-`).
  kAsterisk,     /// Asterisk or multiplication sign (e.g. `*`).
  kSlash,        /// Slash or division operator (e.g. `/`).
  kPercent,      /// Percent or modulo operator (e.g. `%`).
  kLeftParen,    /// Left parenthesis (e.g. `(`).
  kRightParen,   /// Right parenthesis (e.g. `)`).
  kLeftSquare,   /// Left square bracket (e.g. `[`).
  kRightSquare,  /// Right square bracket (e.g. `]`).

  // --- Boolean Literals ---
  kBooleanLit,  /// A boolean literal (`true`, `false`).

  // --- String Literals ---
  kStringLit,  /// A string literal (e.g. `"hello"`).

  // --- Exact Numeric Literals ---
  kBigDecimalLit,  /// A arbitrary-precision signed decimal number.
  kBigIntLit,      /// A 64-bit (4 byte) big integer literal.
  kIntLit,         /// A 32-bit (3 byte) integer literal.
  kSmallIntLit,    /// A 16-bit (2 byte) small integer literal.
  kTinyIntLit,     /// A 8-bit (1 byte) tiny integer literal.

  // --- Approx Numeric Literals ---
  kFloatLit,   /// A floating-point number literal.
  kDoubleLit,  /// A double precision floating-point literal.

  // --- Other ---
  kUnquotedIdent,  /// An unquoted identifier (e.g. variable name).
  kQuotedIdent,    /// A quoted identifier (e.g. `"column"`).

  // --- Special ---
  kEof,  /// End-of-file marker.
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
}  // namespace yuzu::lang

#endif  // LANG_LEXER_TOKEN_KIND_H_
