#ifndef SYNTAX_SYNTAX_KIND_H_
#define SYNTAX_SYNTAX_KIND_H_

#include <cstdint>

namespace orion::syntax {
// Note: The 'tokens' enum values must match TokenKind.
enum class SyntaxKind : uint16_t {
  // --- Trivia ---
  kWhitespace,
  kNewline,
  kComment,

  // --- Keywords ---

  // --- Punctuation ---
  kDot,

  kPlus,
  kMinus,
  kAsterisk,
  kSlash,
  kPercent,

  // --- Boolean Literals ---
  kBooleanLiteral,

  // --- String Literals ---
  kStringLiteral,

  // --- Exact Numeric Literals ---
  kIntLiteral,
  kBigIntLiteral,
  kSmallIntLiteral,
  kTinyIntLiteral,

  // --- Approx Numeric Literals ---
  kFloatLiteral,
  kDoubleLit,
  kBigDecimalLiteral,

  // --- Other ---
  kIdentifier,
  kQuotedIdentifier,

  // --- Special ---
  kEof,

  // --- Nodes ---
  kError,
};
}  // namespace orion::syntax

#endif  // SYNTAX_SYNTAX_KIND_H_
