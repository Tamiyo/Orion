#ifndef ORION_SYNTAX_LEXER_LEXER_H_
#define ORION_SYNTAX_LEXER_LEXER_H_

#include <optional>

#include "syntax/lexer/abstract_lexer.h"
#include "syntax/lexer/token.h"

namespace orion::syntax {
class Lexer final : public AbstractLexer {
 public:
  explicit Lexer(const std::string &source,
                 const bool is_case_sensitive = false)
      : AbstractLexer(source, is_case_sensitive) {}

  virtual ~Lexer() = default;

  std::optional<Token> TryNextToken() override;

 private:
  // Token
  std::optional<Token> TryWhitespace();
  std::optional<Token> TryOperator();
  std::optional<Token> TryKeywordOrIdentifier();
  std::optional<Token> TryLiteral();
  std::optional<Token> TryNumericLiteral(bool consume_digits = true);

  // Fragments
  void ConsumeExponent();
  void ConsumeDigits();
  void ConsumeLetters();
};
}  // namespace orion::syntax
#endif  // ORION_SYNTAX_LEXER_LEXER_H_