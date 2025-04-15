#ifndef LANG_PARSER_SYNTAX_KIND_H_
#define LANG_PARSER_SYNTAX_KIND_H_

#include <cstdint>
#include <string_view>

namespace yuzu::lang {
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

  kInfixExpr,
  kPrefixExpr,
  kPostfixExpr,

  kVariableRef,

  kError,
};

[[nodiscard]] constexpr std::u32string_view SyntaxKindToString(
    const SyntaxKind kind) noexcept {
  switch (kind) {
    case SyntaxKind::kWhitespace:
      return U"Whitespace";
    case SyntaxKind::kNewline:
      return U"Newline";
    case SyntaxKind::kComment:
      return U"Comment";

    case SyntaxKind::kDot:
      return U"Dot";

    case SyntaxKind::kPlus:
      return U"Plus";
    case SyntaxKind::kMinus:
      return U"Minus";
    case SyntaxKind::kAsterisk:
      return U"Asterisk";
    case SyntaxKind::kSlash:
      return U"Slash";
    case SyntaxKind::kPercent:
      return U"Percent";

    case SyntaxKind::kBooleanLiteral:
      return U"BooleanLiteral";

    case SyntaxKind::kStringLiteral:
      return U"StringLiteral";

    case SyntaxKind::kIntLiteral:
      return U"IntLiteral";
    case SyntaxKind::kBigIntLiteral:
      return U"BigIntLiteral";
    case SyntaxKind::kSmallIntLiteral:
      return U"SmallIntLiteral";
    case SyntaxKind::kTinyIntLiteral:
      return U"TinyIntLiteral";

    case SyntaxKind::kFloatLiteral:
      return U"FloatLiteral";
    case SyntaxKind::kDoubleLit:
      return U"DoubleLit";
    case SyntaxKind::kBigDecimalLiteral:
      return U"BigDecimalLiteral";

    case SyntaxKind::kIdentifier:
      return U"Identifier";
    case SyntaxKind::kQuotedIdentifier:
      return U"QuotedIdentifier";

    case SyntaxKind::kEof:
      return U"Eof";

    case SyntaxKind::kRoot:
      return U"Root";

    case SyntaxKind::kInfixExpr:
      return U"InfixExpr";

    case SyntaxKind::kPrefixExpr:
      return U"PrefixExpr";

    case SyntaxKind::kPostfixExpr:
      return U"PostfixExpr";

    case SyntaxKind::kError:
      return U"Error";

    default:
      return U"Unknown";
  }
}
}  // namespace yuzu::lang
#endif  // LANG_PARSER_SYNTAX_KIND_H_
