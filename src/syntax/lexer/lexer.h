#ifndef ORION_SYNTAX_LEXER_LEXER_H_
#define ORION_SYNTAX_LEXER_LEXER_H_

#include <optional>
#include <string>

#include "syntax/lexer/abstract_lexer.h"
#include "syntax/lexer/token.h"

namespace orion::syntax {
class Lexer final : public AbstractLexer {
 public:
  explicit Lexer(const std::u32string &source) : AbstractLexer(source) {}
  Lexer() = delete;

  std::optional<Token> TryNextToken() override;

 private:
  // Token
  std::optional<Token> TryWhitespace();
  std::optional<Token> TryOperator();
  std::optional<Token> TryKeywordOrIdentifier();
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
#endif  // ORION_SYNTAX_LEXER_LEXER_H_