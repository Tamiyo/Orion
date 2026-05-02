#include "yuzu/Lexer/Lexer.h"

#include "yuzu/Lexer/Token.h"
#include "yuzu/Lexer/TokenKind.h"

#include <cassert>
#include <optional>
#include <string>
#include <string_view>

namespace yuzu::lexer {
namespace {
const std::u32string letKeyword = U"let";
const std::u32string mutKeyword = U"mut";
}; // namespace

std::optional<Token> Lexer::getNextToken() {
  if (current >= source.end()) {
    return std::nullopt;
  }

  const char32_t *start = current;
  switch (*current) {
  case U' ':
  case U'\t': {
    bump();
    return createToken(start, TokenKind::Space);
  }
  case U'\n':
  case U'\r': {
    bump();
    return createToken(start, TokenKind::Newline);
  }
  case U'=': {
    bump();
    return createToken(start, TokenKind::Equals);
  }
  case U'+': {
    bump();
    return createToken(start, TokenKind::Plus);
  }
  case U'-': {
    bump();
    return createToken(start, TokenKind::Minus);
  }
  case U'*': {
    bump();
    return createToken(start, TokenKind::Star);
  }
  case U'/': {
    if (peek(1) == U'/') {
      bump(2);
      bumpWhile([](const char32_t *ch) { return *ch != U'\n'; });
      return createToken(start, TokenKind::Comment);
    }
    bump();
    return createToken(start, TokenKind::Slash);
  }
  };

  if (atDigit(current)) {
    bumpWhile([](const char32_t *ch) { return atDigit(ch); });
    return createToken(start, TokenKind::Number);
  }

  if (atIdentStart(current)) {
    bumpWhile([](const char32_t *ch) { return atIdent(ch); });

    const auto ident = std::u32string_view(start, current - start);

    if (ident == letKeyword) {
      return createToken(start, TokenKind::LetKw);
    }

    if (ident == mutKeyword) {
      return createToken(start, TokenKind::MutKw);
    }

    return createToken(start, TokenKind::Ident);
  }

  return std::nullopt;
}
} // namespace yuzu::lexer
