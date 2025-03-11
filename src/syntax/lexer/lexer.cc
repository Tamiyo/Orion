#include "syntax/lexer/lexer.h"

#include <functional>
#include <stdexcept>
#include <string>

#include "syntax/lexer/token.h"
#include "syntax/lexer/token_kind.h"

namespace orion::syntax {
constexpr char kB = 'B';
constexpr char kD = 'D';
constexpr char kE = 'E';
constexpr char kF = 'F';
constexpr char kL = 'L';
constexpr char kS = 'S';
constexpr char kY = 'Y';

constexpr char kQuote = '\'';
constexpr char kDoubleQuote = '"';

constexpr char kSpace = ' ';
constexpr char kNewline = '\n';
constexpr char kTab = '\t';

constexpr char kDot = '.';
constexpr char kUnderscore = '_';

constexpr char kPlus = '+';
constexpr char kMinus = '-';
constexpr char kAsterisk = '*';
constexpr char kSlash = '/';
constexpr char kPercent = '%';

enum class NumericKind {
  kApprox,
  kExact,
};

std::optional<Token> Lexer::TryNextToken() {
  if (const std::optional<Token> whitespace = TryWhitespace();
      whitespace.has_value()) {
    return whitespace;
  }

  if (const std::optional<Token> op = TryOperator(); op.has_value()) {
    return op;
  }

  if (const std::optional<Token> keyword_or_identifier =
          TryKeywordOrIdentifier();
      keyword_or_identifier.has_value()) {
    return keyword_or_identifier;
  }

  if (IsCurrent(kDot)) {
    // Some approximate numerics do not start with a leading digit.
    if (IsCurrent([](const int ch) { return std::isdigit(ch); }, 1)) {
      return TryNumericLiteral(false);
    }

    return ConsumeAndCreateToken(TokenKind::kDot);
  }

  return TryLiteral();
}

std::optional<Token> Lexer::TryWhitespace() {
  if (IsCurrent2(kSpace, kTab)) {
    ConsumeWhile([](const char ch) { return ch == kSpace || ch == kTab; });
    return CreateToken(TokenKind::kWhitespace);
  }

  if (IsCurrent(kNewline)) {
    ConsumeWhile([](const char ch) { return ch == kNewline; });
    return CreateToken(TokenKind::kNewline);
  }

  return std::nullopt;
}

std::optional<Token> Lexer::TryOperator() {
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

std::optional<Token> Lexer::TryKeywordOrIdentifier() {
  // Identifiers must start with a letter or an underscore.
  if (!IsCurrent([](const char ch) {
        return std::isalpha(ch) || ch == kUnderscore;
      })) {
    return std::nullopt;
  }

  ConsumeWhile(
      [](const char ch) { return std::isalnum(ch) || ch == kUnderscore; });

  return CreateToken(TokenKind::kIdentifier);
}

std::optional<Token> Lexer::TryLiteral() {
  if (IsCurrent([](const int ch) { return std::isdigit(ch); })) {
    return TryNumericLiteral();
  }

  return std::nullopt;
}

// https://github.com/apache/spark/blob/master/sql/api/src/main/antlr4/org/apache/spark/sql/catalyst/parser/SqlBaseLexer.g4#L578
std::optional<Token> Lexer::TryNumericLiteral(const bool consume_digits) {
  // TODO : Might need to redesign this consume digits thing...
  if (consume_digits) {
    ConsumeDigits();
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

  if (IsCurrent(kF)) {
    return ConsumeAndCreateToken(TokenKind::kFloatLiteral);
  }

  if (IsCurrent(kB) && IsCurrent(kD, 1)) {
    return ConsumeAndCreateToken(TokenKind::kBigDecimalLiteral, 2);
  }

  if (IsCurrent(kD)) {
    return ConsumeAndCreateToken(TokenKind::kDoubleLit);
  }

  if (IsCurrent(kL)) {
    return ConsumeAndCreateToken(TokenKind::kBigIntLiteral);
  }

  if (IsCurrent(kS)) {
    return ConsumeAndCreateToken(TokenKind::kSmallIntLiteral);
  }

  if (IsCurrent(kY)) {
    return ConsumeAndCreateToken(TokenKind::kTinyIntLiteral);
  }

  if (numericKind == NumericKind::kExact) {
    return CreateToken(TokenKind::kIntLiteral);
  }

  return CreateToken(TokenKind::kFloatLiteral);
}

void Lexer::ConsumeExponent() {
  if (!IsCurrent(kE)) {
    return;
  }

  Consume();                   // Eat 'E'
  TryConsume2(kPlus, kMinus);  // Eat '[+-]?'
  ConsumeDigits();
}

void Lexer::ConsumeDigits() {
  if (AtEnd()) {
    throw std::invalid_argument(
        "expected at least one digit in fragment, but at end");
  }

  if (!isdigit(GetCurrent())) {
    throw std::invalid_argument("expected at least one digit in fragment");
  }

  ConsumeWhile(isdigit);
}

void Lexer::ConsumeLetters() {
  if (AtEnd()) {
    throw std::invalid_argument(
        "expected at least one letter in fragment, but at end");
  }

  if (!isalpha(GetCurrent())) {  // Fix: should check for letters, not digits
    throw std::invalid_argument("expected at least one letter in fragment");
  }

  ConsumeWhile(isalpha);
}
}  // namespace orion::syntax
