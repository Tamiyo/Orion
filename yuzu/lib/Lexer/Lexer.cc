#include "yuzu/Lexer/Lexer.h"

#include "yuzu/Lexer/Token.h"
#include "yuzu/Lexer/TokenKind.h"

#include <cassert>
#include <optional>
#include <string>
#include <string_view>

namespace yuzu::lexer {
namespace {
const std::u32string andKeyword = U"and";
const std::u32string asKeyword = U"as";
const std::u32string distinctKeyword = U"distinct";
const std::u32string dropKeyword = U"drop";
const std::u32string extendKeyword = U"extend";
const std::u32string falseKeyword = U"false";
const std::u32string fnKeyword = U"fn";
const std::u32string fromKeyword = U"from";
const std::u32string inKeyword = U"in";
const std::u32string letKeyword = U"let";
const std::u32string mutKeyword = U"mut";
const std::u32string notKeyword = U"not";
const std::u32string orKeyword = U"or";
const std::u32string renameKeyword = U"rename";
const std::u32string returnKeyword = U"return";
const std::u32string selectKeyword = U"select";
const std::u32string structKeyword = U"struct";
const std::u32string tableKeyword = U"table";
const std::u32string trueKeyword = U"true";
const std::u32string whereKeyword = U"where";
}; // namespace

std::optional<Token> Lexer::getNextToken() {
  if (current >= source.end()) {
    return std::nullopt;
  }

  const char32_t *start = current;

  const auto atDecimalChar = [](const char32_t *ch) {
    return atDigit(ch) || *ch == U'_';
  };

  // Consume an optional `[eE][+-]?DIGITS[DIGITS_]*` exponent suffix. Returns
  // true if an exponent was found, false otherwise. Caller decides what to
  // do if the `e` had no digits after it; here we just treat that as part
  // of whatever number we're in (and let any trailing letter be lexed
  // separately).
  const auto consumeExponent = [&]() -> bool {
    if (peek() != U'e' && peek() != U'E') {
      return false;
    }
    const char32_t *expStart = current;
    bump(); // e or E
    if (peek() == U'+' || peek() == U'-') {
      bump();
    }
    if (current >= source.end() || !atDigit(current)) {
      // `1e` with no digits — back off so `e` becomes a fresh ident on the
      // next call.
      current = expStart;
      return false;
    }
    bumpWhile(atDecimalChar);
    return true;
  };

  switch (*current) {
  case U'"': {
    // Standard double-quoted string. `\\` escapes the next character (so
    // `"a\"b"` is one token); a literal newline terminates without
    // closing — emit `Error` so the parser can recover.
    bump(); // open "
    while (current < source.end() && *current != U'"' && *current != U'\n') {
      if (*current == U'\\' && current + 1 < source.end()) {
        bump(2);
      } else {
        bump();
      }
    }
    if (current < source.end() && *current == U'"') {
      bump(); // close "
      return createToken(start, TokenKind::StringLiteral);
    }
    return createToken(start, TokenKind::Error);
  }
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
  case U'(': {
    bump();
    return createToken(start, TokenKind::LeftParen);
  }
  case U')': {
    bump();
    return createToken(start, TokenKind::RightParen);
  }
  case U'[': {
    bump();
    return createToken(start, TokenKind::LeftBracket);
  }
  case U']': {
    bump();
    return createToken(start, TokenKind::RightBracket);
  }
  case U'{': {
    bump();
    return createToken(start, TokenKind::LeftCurly);
  }
  case U'}': {
    bump();
    return createToken(start, TokenKind::RightCurly);
  }
  case U',': {
    bump();
    return createToken(start, TokenKind::Comma);
  }
  case U':': {
    bump();
    return createToken(start, TokenKind::Colon);
  }
  case U'=': {
    if (peek(1) == U'=') {
      bump(2);
      return createToken(start, TokenKind::EqEq);
    }
    bump();
    return createToken(start, TokenKind::Eq);
  }
  case U'!': {
    if (peek(1) == U'=') {
      bump(2);
      return createToken(start, TokenKind::Neq);
    }
    // A lone `!` is not an operator (negation is the `not` keyword).
    bump();
    return createToken(start, TokenKind::Error);
  }
  case U'+': {
    bump();
    return createToken(start, TokenKind::Plus);
  }
  case U'-': {
    if (peek(1) == U'>') {
      bump(2);
      return createToken(start, TokenKind::Arrow);
    }
    bump();
    return createToken(start, TokenKind::Minus);
  }
  case U'*': {
    if (peek(1) == U'*') {
      bump(2);
      return createToken(start, TokenKind::Pow);
    }
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
  case U'.': {
    // Leading-dot float: `.5`, `.123e-7`. Otherwise a `.` is field access.
    if (current + 1 < source.end() && atDigit(current + 1)) {
      bump(); // .
      bumpWhile(atDecimalChar);
      consumeExponent();
      return createToken(start, TokenKind::FloatLiteral);
    }
    bump();
    return createToken(start, TokenKind::Dot);
  }
  case U'<': {
    if (peek(1) == U'<') {
      bump(2);
      return createToken(start, TokenKind::ShiftLeft);
    }

    if (peek(1) == U'=') {
      bump(2);
      return createToken(start, TokenKind::Lte);
    }

    bump();
    return createToken(start, TokenKind::Lt);
  }

  case U'>': {
    if (peek(1) == U'>') {
      bump(2);
      return createToken(start, TokenKind::ShiftRight);
    }

    if (peek(1) == U'=') {
      bump(2);
      return createToken(start, TokenKind::Gte);
    }

    bump();
    return createToken(start, TokenKind::Gt);
  }
  case U'|': {
    // The query pipe `|>`; a bare `|` is not yet a token.
    if (peek(1) == U'>') {
      bump(2);
      return createToken(start, TokenKind::Pipe);
    }
    bump();
    return createToken(start, TokenKind::Error);
  }
  };

  // Raw string `r"..."` — recognized before the identifier path so the
  // leading `r` doesn't get consumed as part of an ident. Escapes inside
  // a raw string are literal; a newline terminates the literal as an
  // error, matching standard double-quoted strings.
  if (*current == U'r' && current + 1 < source.end() && current[1] == U'"') {
    bump(2); // r"
    while (current < source.end() && *current != U'"' && *current != U'\n') {
      bump();
    }
    if (current < source.end() && *current == U'"') {
      bump();
      return createToken(start, TokenKind::RawStringLiteral);
    }
    return createToken(start, TokenKind::Error);
  }

  if (atDigit(current)) {
    // Hex
    const auto isHexLiteral =
        peek() == U'0' && (peek(1) == U'x' || peek(1) == U'X');
    if (isHexLiteral) {
      bump(2); // bump 0x|0X

      const auto atHexDigit = [](const char32_t *ch) {
        return atDigit(ch) || (*ch >= U'a' && *ch <= U'f') ||
               (*ch >= U'A' && *ch <= U'F');
      };

      if (current < source.end() && atHexDigit(current)) {
        bump(); // first hex digit
        bumpWhile([&atHexDigit](const char32_t *ch) {
          return atHexDigit(ch) || *ch == U'_';
        });

        return createToken(start, TokenKind::HexLiteral);
      }

      return createToken(start, TokenKind::Error);
    }

    // Binary
    const auto isBinaryLiteral =
        peek() == U'0' && (peek(1) == U'b' || peek(1) == U'B');
    if (isBinaryLiteral) {
      bump(2); // bump 0b|0B

      if (current < source.end() && (*current == U'0' || *current == U'1')) {
        bump(); // first binary digit
        bumpWhile([](const char32_t *ch) {
          return *ch == U'0' || *ch == U'1' || *ch == U'_';
        });

        return createToken(start, TokenKind::BinaryLiteral);
      }

      return createToken(start, TokenKind::Error);
    }

    // Decimal integer part.
    bumpWhile(atDecimalChar);

    // Fractional part promotes the literal to a float.
    bool isFloat = false;
    if (peek() == U'.') {
      bump();
      bumpWhile(atDecimalChar);
      isFloat = true;
    }
    // Exponent (`1e10`, `1.5e-5`) also promotes.
    if (consumeExponent()) {
      isFloat = true;
    }

    return createToken(start, isFloat ? TokenKind::FloatLiteral
                                      : TokenKind::IntegerLiteral);
  }

  if (atIdentStart(current)) {
    bumpWhile([](const char32_t *ch) { return atIdent(ch); });

    const auto ident = std::u32string_view(start, current - start);

    if (ident == andKeyword) {
      return createToken(start, TokenKind::AndKw);
    }

    if (ident == asKeyword) {
      return createToken(start, TokenKind::AsKw);
    }

    if (ident == falseKeyword) {
      return createToken(start, TokenKind::BooleanLiteral);
    }

    if (ident == fnKeyword) {
      return createToken(start, TokenKind::FnKw);
    }

    if (ident == fromKeyword) {
      return createToken(start, TokenKind::FromKw);
    }

    if (ident == inKeyword) {
      return createToken(start, TokenKind::InKw);
    }

    if (ident == selectKeyword) {
      return createToken(start, TokenKind::SelectKw);
    }

    if (ident == structKeyword) {
      return createToken(start, TokenKind::StructKw);
    }

    if (ident == tableKeyword) {
      return createToken(start, TokenKind::TableKw);
    }

    if (ident == orKeyword) {
      return createToken(start, TokenKind::OrKw);
    }

    if (ident == returnKeyword) {
      return createToken(start, TokenKind::ReturnKw);
    }

    if (ident == mutKeyword) {
      return createToken(start, TokenKind::MutKw);
    }

    if (ident == letKeyword) {
      return createToken(start, TokenKind::LetKw);
    }

    if (ident == notKeyword) {
      return createToken(start, TokenKind::NotKw);
    }

    if (ident == trueKeyword) {
      return createToken(start, TokenKind::BooleanLiteral);
    }

    if (ident == whereKeyword) {
      return createToken(start, TokenKind::WhereKw);
    }

    if (ident == distinctKeyword) {
      return createToken(start, TokenKind::DistinctKw);
    }

    if (ident == dropKeyword) {
      return createToken(start, TokenKind::DropKw);
    }

    if (ident == extendKeyword) {
      return createToken(start, TokenKind::ExtendKw);
    }

    if (ident == renameKeyword) {
      return createToken(start, TokenKind::RenameKw);
    }

    return createToken(start, TokenKind::Identifier);
  }

  // Unknown character: emit a single-character `Error` token and advance
  // past it so the lexer keeps making progress. Returning nullopt here
  // would let `getTokens` mistake the bad byte for end-of-input and drop
  // every following token.
  bump();
  return createToken(start, TokenKind::Error);
}
} // namespace yuzu::lexer
