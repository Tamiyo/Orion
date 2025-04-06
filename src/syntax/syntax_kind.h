#ifndef SYNTAX_SYNTAX_KIND_H_
#define SYNTAX_SYNTAX_KIND_H_

#include <cstdint>
#include <sstream>
#include <string_view>

namespace orion::syntax {
// Note: The 'tokens' enum values must match TokenKind.
enum class SyntaxKind : uint16_t {
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

  // --- Nodes ---
  /// Represents an error node.
  kRoot,
  kBinaryExpr,
  kError,
};

[[nodiscard]] constexpr std::u32string_view SyntaxKindToString(
    SyntaxKind kind) noexcept;

std::basic_ostringstream<char32_t>& operator<<(
    std::basic_ostringstream<char32_t>& oss, SyntaxKind kind) noexcept;
}  // namespace orion::syntax
#endif  // SYNTAX_SYNTAX_KIND_H_
