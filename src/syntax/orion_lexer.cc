#include "syntax/orion_lexer.h"

#include <cwctype>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>

#include "syntax/lexer/token.h"
#include "syntax/lexer/token_kind.h"

namespace orion::syntax {
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

constexpr char32_t kDot = U'.';
constexpr char32_t kUnderscore = U'_';
constexpr char32_t kPlus = U'+';
constexpr char32_t kMinus = U'-';
constexpr char32_t kAsterisk = U'*';
constexpr char32_t kSlash = U'/';
constexpr char32_t kPercent = U'%';

const std::u32string kTrueKeyword = U"true";
const std::u32string kFalseKeyword = U"false";

constexpr char32_t kAsciiMaxCodepoint = 0x7F;

enum class NumericKind {
  kApprox,
  kExact,
};

std::optional<Token> OrionLexer::TryNextToken() noexcept {
  if (const std::optional<Token> whitespace = TryWhitespace();
      whitespace.has_value()) {
    return whitespace;
  }

  if (const std::optional<Token> op = TryOperator(); op.has_value()) {
    return op;
  }
  if (const std::optional<Token> boolean_literal = TryBooleanLiteral();
      boolean_literal.has_value()) {
    return boolean_literal;
  }

  if (const std::optional<Token> keyword_or_identifier =
          TryKeywordOrIdentifier();
      keyword_or_identifier.has_value()) {
    return keyword_or_identifier;
  }

  if (At(kDot)) {
    // Some approximate numerics do not start with a leading digit.
    if (At([](const char32_t ch) { return std::iswdigit(ch); }, 1)) {
      return TryNumericLiteral(false);
    }

    return ConsumeAndCreateToken(TokenKind::kDot);
  }

  return TryLiteral();
}

std::optional<Token> OrionLexer::TryWhitespace() {
  if (At2(kSpace, kTab)) {
    ConsumeWhile([](const char32_t ch) { return ch == kSpace || ch == kTab; });
    return CreateToken(TokenKind::kWhitespace);
  }

  if (At(kNewline)) {
    ConsumeWhile([](const char32_t ch) { return ch == kNewline; });
    return CreateToken(TokenKind::kNewline);
  }

  return std::nullopt;
}

std::optional<Token> OrionLexer::TryOperator() {
  switch (GetCurrent()) {
    case kPlus:
      return ConsumeAndCreateToken(TokenKind::kPlus);

    case kMinus:
      return ConsumeAndCreateToken(TokenKind::kMinus);

    case kAsterisk:
      return ConsumeAndCreateToken(TokenKind::kAsterisk);

    case kSlash:
      return ConsumeAndCreateToken(TokenKind::kSlash);

    case kPercent:
      return ConsumeAndCreateToken(TokenKind::kPercent);

    default:
      return std::nullopt;
  }
}

std::optional<Token> OrionLexer::TryKeywordOrIdentifier() {
  if (const std::optional<Token> quoted_identifier = TryQuotedIdentifier();
      quoted_identifier.has_value()) {
    return quoted_identifier;
  }

  return TryIdentifier();
}

std::optional<Token> OrionLexer::TryQuotedIdentifier() {
  if (!At(kBacktick)) {
    return std::nullopt;
  }

  Consume();  // Eat '`'

  bool is_delimited = false;
  while (!AtEnd()) {
    if (At(kBacktick) && At(kBacktick, 1)) {
      Consume(2);  // Eat '``'
    } else if (At(kBacktick)) {
      is_delimited = true;
      Consume();  // Eat '`'
      break;
    } else {
      Consume();  // Eat char.
    }
  }

  if (!is_delimited) {
    throw std::invalid_argument("unclosed quoted identifier");
  }

  return CreateToken(TokenKind::kQuotedIdentifier);
}

std::optional<Token> OrionLexer::TryIdentifier() {
  // Identifiers must start with a letter or an underscore.
  if (!At([](const char32_t ch) {
        return std::iswalpha(ch) || ch == kUnderscore ||
               ch > kAsciiMaxCodepoint;
      })) {
    return std::nullopt;
  }

  ConsumeWhile([](const char32_t ch) {
    return std::iswalnum(ch) || ch == kUnderscore || ch > kAsciiMaxCodepoint;
  });

  return CreateToken(TokenKind::kIdentifier);
}

std::optional<Token> OrionLexer::TryLiteral() {
  if (At([](const char32_t ch) { return std::iswdigit(ch); })) {
    return TryNumericLiteral();
  }

  if (At(kDoubleQuote)) {
    return TryStringLiteral();
  }

  return std::nullopt;
}

// '"' ( ~('"'|'\\') | ('\\' .) )* '"'
std::optional<Token> OrionLexer::TryStringLiteral() {
  constexpr char32_t delimiter = kDoubleQuote;

  if (!At(delimiter)) {
    return std::nullopt;
  }

  Consume();  // Eat delimiter.

  bool is_escaped = false;
  ConsumeWhile([this, is_escaped](const char32_t ch) mutable {
    if (is_escaped) {
      switch (GetCurrent()) {
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

  if (!At(delimiter)) {
    throw std::invalid_argument("unclosed string literal");
  }

  Consume();  // Eat delimiter.
  return CreateToken(TokenKind::kStringLiteral);
}

std::optional<Token> OrionLexer::TryBooleanLiteral() {
  if (At(kTrueKeyword)) {
    return ConsumeAndCreateToken(TokenKind::kBooleanLiteral, 4);
  }

  if (At(kFalseKeyword)) {
    return ConsumeAndCreateToken(TokenKind::kBooleanLiteral, 5);
  }

  return std::nullopt;
}

// https://github.com/apache/spark/blob/master/sql/api/src/main/antlr4/org/apache/spark/sql/catalyst/parser/SqlBaseLexer.g4#L578
std::optional<Token> OrionLexer::TryNumericLiteral(const bool consume_digits) {
  if (consume_digits) {
    ConsumeDigits();

    // If there are no more digits, there is nothing else to consume. We're at
    // the end of our input.
    if (AtEnd()) {
      return CreateToken(TokenKind::kIntLiteral);
    }
  }

  NumericKind numericKind;
  switch (GetCurrent()) {
    case kDot: {
      Consume();  // Eat '.'

      numericKind = NumericKind::kApprox;
      ConsumeDigits();
      ConsumeExponent();
      break;
    }
    default: {
      numericKind = NumericKind::kExact;
      ConsumeExponent();
      break;
    }
  }

  if (At(kFUpper) || At(kFLower)) {
    return ConsumeAndCreateToken(TokenKind::kFloatLiteral);
  }

  if ((At(kBUpper) && At(kDUpper, 1)) || (At(kBLower) && At(kDLower, 1))) {
    return ConsumeAndCreateToken(TokenKind::kBigDecimalLiteral, 2);
  }

  if (At(kDUpper) || At(kDLower)) {
    return ConsumeAndCreateToken(TokenKind::kDoubleLit);
  }

  if (At(kLUpper) || At(kLLower)) {
    return ConsumeAndCreateToken(TokenKind::kBigIntLiteral);
  }

  if (At(kSUpper) || At(kSLower)) {
    return ConsumeAndCreateToken(TokenKind::kSmallIntLiteral);
  }

  if (At(kYUpper) || At(kYLower)) {
    return ConsumeAndCreateToken(TokenKind::kTinyIntLiteral);
  }

  if (numericKind == NumericKind::kExact) {
    return CreateToken(TokenKind::kIntLiteral);
  }

  return CreateToken(TokenKind::kFloatLiteral);
}

// Grammar: E[+-]? DIGITS
void OrionLexer::ConsumeExponent() {
  if (!(At(kEUpper) || At(kELower))) {
    return;
  }

  Consume();                   // Eat 'E'
  TryConsume2(kPlus, kMinus);  // Eat '[+-]?'
  ConsumeDigits();
}

// Grammar: [0-9]+
void OrionLexer::ConsumeDigits() {
  if (AtEnd()) {
    throw std::invalid_argument(
        "expected at least one digit in fragment, but at end");
  }

  if (!std::iswdigit(GetCurrent())) {
    throw std::invalid_argument("expected at least one digit in fragment");
  }

  ConsumeWhile([](const char32_t ch) { return std::iswdigit(ch); });
}

// Grammar: [a-zA-Z]+
void OrionLexer::ConsumeLetters() {
  if (AtEnd()) {
    throw std::invalid_argument(
        "expected at least one letter in fragment, but at end");
  }

  if (!std::iswalpha(
          GetCurrent())) {  // Fix: should check for letters, not digits
    throw std::invalid_argument("expected at least one letter in fragment");
  }

  ConsumeWhile([](const char32_t ch) { return std::iswalpha(ch); });
}
}  // namespace orion::syntax
