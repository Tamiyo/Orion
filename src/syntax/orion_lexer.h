#ifndef SYNTAX_ORION_LEXER_H_
#define SYNTAX_ORION_LEXER_H_

#include <optional>
#include <string>

#include "syntax/lexer/lexer.h"
#include "syntax/lexer/token.h"

namespace orion::syntax {
class OrionLexer final : public Lexer {
 public:
  explicit OrionLexer(const std::u32string &source) : Lexer(source) {}
  OrionLexer() = delete;

 protected:
  std::optional<Token> TryNextToken() noexcept override;

 private:
  // Token
  std::optional<Token> TryWhitespace();
  std::optional<Token> TryOperator();
  std::optional<Token> TryKeywordOrIdentifier();
  std::optional<Token> TryQuotedIdentifier();
  std::optional<Token> TryIdentifier();
  std::optional<Token> TryLiteral();
  std::optional<Token> TryStringLiteral();
  std::optional<Token> TryBooleanLiteral();
  std::optional<Token> TryNumericLiteral(bool consume_digits = true);

  // Fragments
  void ConsumeExponent();
  void ConsumeDigits();
  void ConsumeLetters();
};
}  // namespace orion::syntax
#endif  // SYNTAX_ORION_LEXER_H_
