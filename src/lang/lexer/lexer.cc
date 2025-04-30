#include "lang/lexer/lexer.h"

#include <cwctype>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

#include "lang/lexer/token_kind.h"

namespace yuzu::lang {
constexpr char32_t kBUpper = U'B';
constexpr char32_t kDUpper = U'D';
constexpr char32_t kEUpper = U'E';
constexpr char32_t kFUpper = U'F';
constexpr char32_t kLUpper = U'L';
constexpr char32_t kSUpper = U'S';
constexpr char32_t kYUpper = U'Y';

constexpr char32_t kBLower = U'b';
constexpr char32_t kDLower = U'd';
constexpr char32_t kELower = U'e';
constexpr char32_t kFLower = U'f';
constexpr char32_t kLLower = U'l';
constexpr char32_t kNLower = U'n';
constexpr char32_t kRLower = U'r';
constexpr char32_t kSLower = U's';
constexpr char32_t kTLower = U't';
constexpr char32_t kYLower = U'y';

constexpr char32_t kQuote = '\'';
constexpr char32_t kDoubleQuote = '"';
constexpr char32_t kBackslash = '\\';
constexpr char32_t kBacktick = '`';

constexpr char32_t kSpace = U' ';
constexpr char32_t kNewline = U'\n';
constexpr char32_t kTab = U'\t';

constexpr char32_t kLeftParen = U'(';
constexpr char32_t kRightParen = U')';
constexpr char32_t kLeftSquare = U'[';
constexpr char32_t kRightSquare = U']';

constexpr char32_t kDot = U'.';
constexpr char32_t kUnderscore = U'_';
constexpr char32_t kPlus = U'+';
constexpr char32_t kMinus = U'-';
constexpr char32_t kAsterisk = U'*';
constexpr char32_t kSlash = U'/';
constexpr char32_t kPercent = U'%';

constexpr std::u32string_view kTrueKeyword = U"true";
constexpr std::u32string_view kFalseKeyword = U"false";

constexpr char32_t kAsciiMaxCodepoint = 0x7F;

enum class NumericKind {
  kApprox,
  kExact,
};

std::optional<Lexer::Token> Lexer::TryNextToken() noexcept {
  if (const std::optional<Lexer::Token> whitespace = TryWhitespace(this);
      whitespace.has_value()) {
    return whitespace;
  }

  if (const std::optional<Lexer::Token> op = TryPunctuation(this);
      op.has_value()) {
    return op;
  }

  if (const std::optional<Lexer::Token> op = TryOperator(this);
      op.has_value()) {
    return op;
  }

  if (const std::optional<Lexer::Token> boolean_literal =
          TryBooleanLiteral(this);
      boolean_literal.has_value()) {
    return boolean_literal;
  }

  if (const std::optional<Lexer::Token> keyword_or_identifier =
          TryKeywordOrIdentifier(this);
      keyword_or_identifier.has_value()) {
    return keyword_or_identifier;
  }

  if (At(kDot)) {
    // Some approximate numerics do not start with a leading digit.
    if (At([](const char32_t ch) { return std::iswdigit(ch); }, 1)) {
      return TryNumericLiteral(this, false);
    }

    return BumpAndCreateToken(TokenKind::kDot);
  }

  return TryLiteral(this);
}

std::optional<Lexer::Token> TryWhitespace(Lexer* l) {
  if (l->At2(kSpace, kTab)) {
    l->BumpWhile([](const char32_t ch) { return ch == kSpace || ch == kTab; });
    return l->CreateToken(TokenKind::kWhitespace);
  }

  if (l->At(kNewline)) {
    l->BumpWhile([](const char32_t ch) { return ch == kNewline; });
    return l->CreateToken(TokenKind::kNewline);
  }

  return std::nullopt;
}

std::optional<Lexer::Token> TryPunctuation(Lexer* l) {
  switch (l->GetCurrent()) {
    case kLeftParen:
      return l->BumpAndCreateToken(TokenKind::kLeftParen);
    case kRightParen:
      return l->BumpAndCreateToken(TokenKind::kRightParen);
    case kLeftSquare:
      return l->BumpAndCreateToken(TokenKind::kLeftSquare);
    case kRightSquare:
      return l->BumpAndCreateToken(TokenKind::kRightSquare);
    default:
      return std::nullopt;
  }
}

std::optional<Lexer::Token> TryOperator(Lexer* l) {
  switch (l->GetCurrent()) {
    case kPlus:
      return l->BumpAndCreateToken(TokenKind::kPlus);
    case kMinus:
      return l->BumpAndCreateToken(TokenKind::kMinus);
    case kAsterisk:
      return l->BumpAndCreateToken(TokenKind::kAsterisk);
    case kSlash:
      return l->BumpAndCreateToken(TokenKind::kSlash);
    case kPercent:
      return l->BumpAndCreateToken(TokenKind::kPercent);
    default:
      return std::nullopt;
  }
}

std::optional<Lexer::Token> TryKeywordOrIdentifier(Lexer* l) {
  if (const std::optional<Lexer::Token> quoted_identifier =
          TryQuotedIdentifier(l);
      quoted_identifier.has_value()) {
    return quoted_identifier;
  }

  return TryIdentifier(l);
}

std::optional<Lexer::Token> TryQuotedIdentifier(Lexer* l) {
  if (!l->At(kBacktick)) {
    return std::nullopt;
  }

  l->Bump();  // Eat '`'

  bool is_delimited = false;
  while (!l->AtEnd()) {
    if (l->At(kBacktick) && l->At(kBacktick, 1)) {
      l->Bump(2);  // Eat '``'
    } else if (l->At(kBacktick)) {
      is_delimited = true;
      l->Bump();  // Eat '`'
      break;
    } else {
      l->Bump();  // Eat char.
    }
  }

  if (!is_delimited) {
    throw std::invalid_argument("unclosed quoted identifier");
  }

  return l->CreateToken(TokenKind::kQuotedIdent);
}

std::optional<Lexer::Token> TryIdentifier(Lexer* l) {
  // Identifiers must start with a letter or an underscore.
  if (!l->At([](const char32_t ch) {
        return std::iswalpha(ch) || ch == kUnderscore ||
               ch > kAsciiMaxCodepoint;
      })) {
    return std::nullopt;
  }

  l->BumpWhile([](const char32_t ch) {
    return std::iswalnum(ch) || ch == kUnderscore || ch > kAsciiMaxCodepoint;
  });

  return l->CreateToken(TokenKind::kUnquotedIdent);
}

std::optional<Lexer::Token> TryLiteral(Lexer* l) {
  if (l->At([](const char32_t ch) { return std::iswdigit(ch); })) {
    return TryNumericLiteral(l);
  }

  if (l->At(kDoubleQuote)) {
    return TryStringLiteral(l);
  }

  return std::nullopt;
}

// '"' ( ~('"'|'\\') | ('\\' .) )* '"'
std::optional<Lexer::Token> TryStringLiteral(Lexer* l) {
  constexpr char32_t delimiter = kDoubleQuote;

  if (!l->At(delimiter)) {
    return std::nullopt;
  }

  l->Bump();  // Eat delimiter.

  bool is_escaped = false;
  l->BumpWhile([l, is_escaped](const char32_t ch) mutable {
    if (is_escaped) {
      switch (l->GetCurrent()) {
        case kTLower:
        case kBLower:
        case kNLower:
        case kRLower:
        case kFLower:
        case kQuote:
        case kDoubleQuote:
        case kBackslash:
          is_escaped = false;
          return true;
        default:
          throw std::invalid_argument("invalid escape sequence");
      }
    }

    if (ch == kBackslash) {
      is_escaped = true;
      return true;
    }

    return ch != delimiter;
  });

  if (!l->At(delimiter)) {
    throw std::invalid_argument("unclosed string literal");
  }

  l->Bump();  // Eat delimiter.
  return l->CreateToken(TokenKind::kStringLit);
}

std::optional<Lexer::Token> TryBooleanLiteral(Lexer* l) {
  if (l->At(kTrueKeyword)) {
    return l->BumpAndCreateToken(TokenKind::kBooleanLit, 4);
  }

  if (l->At(kFalseKeyword)) {
    return l->BumpAndCreateToken(TokenKind::kBooleanLit, 5);
  }

  return std::nullopt;
}

// https://github.com/apache/spark/blob/master/sql/api/src/main/antlr4/org/apache/spark/sql/catalyst/parser/SqlBaseLexer.g4#L578
std::optional<Lexer::Token> TryNumericLiteral(Lexer* l,
                                              const bool consume_digits) {
  if (consume_digits) {
    BumpDigits(l);

    // If there are no more digits, there is nothing else to consume. We're at
    // the end of our input.
    if (l->AtEnd()) {
      return l->CreateToken(TokenKind::kIntLit);
    }
  }

  NumericKind numericKind;
  switch (l->GetCurrent()) {
    case kDot: {
      l->Bump();  // Eat '.'

      numericKind = NumericKind::kApprox;
      BumpDigits(l);
      BumpExponent(l);
      break;
    }
    default: {
      numericKind = NumericKind::kExact;
      BumpExponent(l);
      break;
    }
  }

  if (l->At(kFUpper) || l->At(kFLower)) {
    return l->BumpAndCreateToken(TokenKind::kFloatLit);
  }

  if ((l->At(kBUpper) && l->At(kDUpper, 1)) ||
      (l->At(kBLower) && l->At(kDLower, 1))) {
    return l->BumpAndCreateToken(TokenKind::kBigDecimalLit, 2);
  }

  if (l->At(kDUpper) || l->At(kDLower)) {
    return l->BumpAndCreateToken(TokenKind::kDoubleLit);
  }

  if (l->At(kLUpper) || l->At(kLLower)) {
    return l->BumpAndCreateToken(TokenKind::kBigIntLit);
  }

  if (l->At(kSUpper) || l->At(kSLower)) {
    return l->BumpAndCreateToken(TokenKind::kSmallIntLit);
  }

  if (l->At(kYUpper) || l->At(kYLower)) {
    return l->BumpAndCreateToken(TokenKind::kTinyIntLit);
  }

  if (numericKind == NumericKind::kExact) {
    return l->CreateToken(TokenKind::kIntLit);
  }

  return l->CreateToken(TokenKind::kFloatLit);
}

// Grammar: E[+-]? DIGITS
void BumpExponent(Lexer* l) {
  if (!(l->At(kEUpper) || l->At(kELower))) {
    return;
  }

  l->Bump();                   // Eat 'E'
  l->TryBump2(kPlus, kMinus);  // Eat '[+-]?'
  BumpDigits(l);
}

// Grammar: [0-9]+
void BumpDigits(Lexer* l) {
  if (l->AtEnd()) {
    throw std::invalid_argument(
        "expected at least one digit in fragment, but at end");
  }

  if (!std::iswdigit(l->GetCurrent())) {
    throw std::invalid_argument("expected at least one digit in fragment");
  }

  l->BumpWhile([](const char32_t ch) { return std::iswdigit(ch); });
}

// Grammar: [a-zA-Z]+
void BumpLetters(Lexer* l) {
  if (l->AtEnd()) {
    throw std::invalid_argument(
        "expected at least one letter in fragment, but at end");
  }

  if (!std::iswalpha(
          l->GetCurrent())) {  // Fix: should check for letters, not digits
    throw std::invalid_argument("expected at least one letter in fragment");
  }

  l->BumpWhile([](const char32_t ch) { return std::iswalpha(ch); });
}
}  // namespace yuzu::lang
