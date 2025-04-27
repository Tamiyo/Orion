#ifndef LANG_LEXER_LEXER_H_
#define LANG_LEXER_LEXER_H_

#include <optional>
#include <string_view>

#include "lang/lexer/token.h"
#include "lang/lexer/token_kind.h"
#include "syntax/lexer/lexer.h"

namespace yuzu::lang {

class Lexer final : public syntax::Lexer<TokenKind> {
 public:
  explicit Lexer(const std::u32string source)
      : syntax::Lexer<TokenKind>(source) {}

  Lexer() = delete;

 protected:
  std::optional<Token> TryNextToken() noexcept override;
};

std::optional<Token> TryWhitespace(Lexer* l);
std::optional<Token> TryPunctuation(Lexer* l);
std::optional<Token> TryOperator(Lexer* l);
std::optional<Token> TryKeywordOrIdentifier(Lexer* l);
std::optional<Token> TryQuotedIdentifier(Lexer* l);
std::optional<Token> TryIdentifier(Lexer* l);
std::optional<Token> TryLiteral(Lexer* l);
std::optional<Token> TryStringLiteral(Lexer* l);
std::optional<Token> TryBooleanLiteral(Lexer* l);
std::optional<Token> TryNumericLiteral(Lexer* l, bool consume_digits = true);

void BumpExponent(Lexer* l);
void BumpDigits(Lexer* l);
void BumpLetters(Lexer* l);
}  // namespace yuzu::lang

#endif  // LANG_LEXER_LEXER_H_
