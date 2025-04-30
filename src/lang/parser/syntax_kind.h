#ifndef LANG_PARSER_SYNTAX_KIND_H_
#define LANG_PARSER_SYNTAX_KIND_H_

#include <array>
#include <cstdint>
#include <string_view>

namespace yuzu::lang {
// Note: The 'tokens' enum values must match TokenKind.
enum class SyntaxKind : uint16_t {
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
  kBooleanLiteral,  /// A boolean literal (`true`, `false`).

  // --- String Literals ---
  kStringLiteral,  /// A string literal (e.g. `"hello"`).

  // --- Exact Numeric Literals ---
  kBigDecimalLit,  /// A arbitrary-precision signed decimal number.
  kBigIntLit,      /// A 64-bit (4 byte) big integer literal.
  kIntLit,         /// A 32-bit (3 byte) integer literal.
  kSmallIntLit,    /// A 16-bit (2 byte) small integer literal.
  kTinyIntLit,     /// A 8-bit (1 byte) tiny integer literal.

  // --- Approx Numeric Literals ---
  kFloatLit,  /// A floating-point number literal.
  kDoubleLit,     /// A double precision floating-point literal.

  // --- Other ---
  kUnquotedIdent,  /// An unquoted identifier (e.g. variable name).
  kQuotedIdent,    /// A quoted identifier (e.g. `"column"`).

  // --- Special ---
  kEof,  /// End-of-file marker.

  ///////////////////////////////////////////////////////

  // --- Root Nodes ---
  kRoot,

  // --- Expression Nodes ---
  kInfixExpr,
  kPrefixExpr,
  kPostfixExpr,
  kParenExpr,

  // --- Infix Operators ---
  kAdd,
  kSub,
  kMul,
  kDiv,
  kMod,

  // --- Postfix Operators ---
  kIndex,

  // --- Literal Nodes ---
  kIdent,
  kLiteral,

  // --- Special Nodes ---
  kError,  /// Represents an error node.
  k_LAST_  /// The last value in the SyntaxKind enum. Reserved for internal
           /// use.
};

inline constexpr std::array<std::u32string_view,
                            static_cast<size_t>(SyntaxKind::k_LAST_)>
    kSyntaxKindNames = {
        // --- Trivia ---
        U"Whitespace", U"Newline", U"Comment",

        // --- Punctuation ---
        U"Dot", U"Plus", U"Minus", U"Asterisk", U"Slash", U"Percent",
        U"LeftParen", U"RightParen", U"LeftSquare", U"RightSquare",

        // --- Boolean Literals ---
        U"BooleanLit",

        // --- String Literals ---
        U"StringLit",

        // --- Exact Numeric Literals ---
        U"BigDecimalLit", U"BigIntLit", U"IntLit",
        U"SmallIntLit", U"TinyIntLit",

        // --- Approx Numeric Literals ---
        U"FloatLit", U"DoubleLit",

        // --- Other ---
        U"UnquotedIdent", U"QuotedIdent",

        // --- Special ---
        U"Eof",

        // --- Root Nodes ---
        U"Root",

        // --- Expression Nodes ---
        U"InfixExpr", U"PrefixExpr", U"PostfixExpr", U"ParenExpr",

        // --- Infix Operators ---
        U"Add", U"Sub", U"Mul", U"Div", U"Mod",
        // --- Postfix Operators ---
        U"Index",

        // --- Literal Nodes ---
        U"Ident", U"Literal",

        // --- Special Nodes ---
        U"Error"};

[[nodiscard]] inline constexpr std::u32string_view ToU32String(
    SyntaxKind kind) noexcept {
  const auto index = static_cast<size_t>(kind);
  if (index >= kSyntaxKindNames.size()) return U"Unknown";
  return kSyntaxKindNames[index];
}
}  // namespace yuzu::lang
#endif  // LANG_PARSER_SYNTAX_KIND_H_
