#ifndef ORION_SYNTAX_LEXER_TOKEN_KIND_H_
#define ORION_SYNTAX_LEXER_TOKEN_KIND_H_

#include <cstdint>
#include <string>

namespace orion::syntax {
enum class TokenKind : uint16_t {
  // Trivia
  kWhitespace,
  kNewline,
  kComment,

  // Keywords

  // Punctuation
  kDot,

  kPlus,
  kMinus,
  kAsterisk,
  kSlash,
  kPercent,

  // String Literals
  // kStringLiteral,

  // Exact Numeric Literals
  kIntLiteral,
  kBigIntLiteral,
  kSmallIntLiteral,
  kTinyIntLiteral,

  // Approx Numeric Literals
  kFloatLiteral,
  kDoubleLit,
  kBigDecimalLiteral,

  // Other
  kIdentifier,

  // Special
  kEof,
};
}  // namespace orion::syntax
#endif  // ORION_SYNTAX_LEXER_TOKEN_KIND_H_
