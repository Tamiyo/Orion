#include "yuzu/Lexer/Lexer.h"

#include "yuzu/Lexer/Range.h"
#include "yuzu/Lexer/Token.h"
#include "yuzu/Lexer/TokenKind.h"

#include <cassert>
#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>

namespace yuzu::lexer {

Token Lexer::createToken(const char32_t *start, TokenKind kind) const noexcept {
  const ptrdiff_t startIndex = (start - source.begin());
  const ptrdiff_t endIndex = (current - source.begin());

  assert(startIndex >= 0 && "Token start cannot be negative");
  assert(endIndex >= 0 && "Token end cannot be negative");

  assert(startIndex <= std::numeric_limits<uint32_t>::max() &&
         "Token start exceeds uint32_t range");
  assert(endIndex <= std::numeric_limits<uint32_t>::max() &&
         "Token end exceeds uint32_t range");

  const auto source = std::u32string_view(start, current - start);
  const auto range = Range{
      .start = static_cast<uint32_t>(startIndex),
      .end = static_cast<uint32_t>(endIndex),
  };

  return Token(kind, source, range);
}

std::optional<Token> Lexer::getNextToken() noexcept {
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

  if (atAlpha(current)) {
    bumpWhile([](const char32_t *ch) { return atIdent(ch); });
    return createToken(start, TokenKind::Ident);
  }

  return std::nullopt;
}
} // namespace yuzu::lexer
