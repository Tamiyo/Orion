#include <gtest/gtest.h>

#include <optional>
#include <string>

#include "syntax/lexer/lexer.h"
#include "syntax/lexer/span.h"
#include "syntax/lexer/token_kind.h"

namespace orion::syntax {
std::optional<Token> BuildToken(const TokenKind kind, const size_t start,
                                const size_t stop,
                                const std::u32string& source) {
  return std::make_optional(
      Token(static_cast<uint16_t>(kind), Span(start, stop), source));
}

std::optional<Token> BuildToken(const TokenKind kind,
                                const std::u32string& source) {
  return BuildToken(kind, 0, source.length(), source);
}
}  // namespace orion::syntax

namespace {
struct SingleTokenTestCase {
  orion::syntax::TokenKind kind;
  std::u32string source;
  std::string test_name;
};

class SingleTokenParameterizedTestFixture
    : public ::testing::TestWithParam<SingleTokenTestCase> {};

INSTANTIATE_TEST_SUITE_P(
    LexerTest, SingleTokenParameterizedTestFixture,
    ::testing::Values(
        // Keywords

        // Operators
        SingleTokenTestCase{orion::syntax::TokenKind::kPlus, U"+", "Plus"},
        SingleTokenTestCase{orion::syntax::TokenKind::kMinus, U"-", "Minus"},
        SingleTokenTestCase{orion::syntax::TokenKind::kAsterisk, U"*",
                            "Asterisk"},
        SingleTokenTestCase{orion::syntax::TokenKind::kSlash, U"/", "Slash"},
        SingleTokenTestCase{orion::syntax::TokenKind::kPercent, U"%",
                            "Percent"},

        // Identifiers
        SingleTokenTestCase{orion::syntax::TokenKind::kIdentifier, U"_",
                            "IdentifierUnderscore"},
        SingleTokenTestCase{orion::syntax::TokenKind::kIdentifier, U"_a",
                            "IdentifierUnderscoreletter"},
        SingleTokenTestCase{orion::syntax::TokenKind::kIdentifier, U"_1",
                            "IdentifierUnderscoreDigit"},
        SingleTokenTestCase{orion::syntax::TokenKind::kIdentifier, U"_a1",
                            "IdentifierUnderscoreLetterDigit"},
        SingleTokenTestCase{orion::syntax::TokenKind::kIdentifier, U"_1a",
                            "IdentifierUnderscoreDigitLetter"},
        SingleTokenTestCase{orion::syntax::TokenKind::kIdentifier, U"h",
                            "IdentifierShort"},
        SingleTokenTestCase{orion::syntax::TokenKind::kIdentifier, U"hhhhh",
                            "IdentifierLong"},
        SingleTokenTestCase{orion::syntax::TokenKind::kIdentifier, U"h1",
                            "IdentifierWithDigitsShort"},
        SingleTokenTestCase{orion::syntax::TokenKind::kIdentifier,
                            U"hg314141gas151fafsg1",
                            "IdentifierWithDigitsLong"},
        SingleTokenTestCase{orion::syntax::TokenKind::kIdentifier,
                            U"_AA_BB_112abG_51", "IdentifierMixed"},

        // Unicode Identifiers
        SingleTokenTestCase{orion::syntax::TokenKind::kIdentifier, U"🍕",
                            "UnicodeIdentifier"},
        SingleTokenTestCase{orion::syntax::TokenKind::kIdentifier,
                            U"伂告伒伄伌伜", "UnicodeIdentifierMultipleChars"},

        // String Literals
        SingleTokenTestCase{orion::syntax::TokenKind::kStringLiteral,
                            U"\"Hello World\"", "StringLiteral"},
        SingleTokenTestCase{orion::syntax::TokenKind::kStringLiteral,
                            U"\"Hello \\t World\"",
                            "StringLiteralWithTabEscapedCharacter"},
        SingleTokenTestCase{orion::syntax::TokenKind::kStringLiteral,
                            U"\"Hello \\b World\"",
                            "StringLiteralWithBackspaceEscapedCharacter"},
        SingleTokenTestCase{orion::syntax::TokenKind::kStringLiteral,
                            U"\"Hello \\n World\"",
                            "StringLiteralWithNewlineEscapedCharacter"},
        SingleTokenTestCase{orion::syntax::TokenKind::kStringLiteral,
                            U"\"Hello \\r World\"",
                            "StringLiteralWithCarriageReturnEscapedCharacter"},
        SingleTokenTestCase{orion::syntax::TokenKind::kStringLiteral,
                            U"\"Hello \\f World\"",
                            "StringLiteralWithFormFeedEscapedCharacter"},
        SingleTokenTestCase{orion::syntax::TokenKind::kStringLiteral,
                            U"\"Hello \\' World\"",
                            "StringLiteralWithQuoteEscapedCharacter"},
        SingleTokenTestCase{orion::syntax::TokenKind::kStringLiteral,
                            U"\"Hello \\\" World\"",
                            "StringLiteralWithDoubleQuoteEscapedCharacter"},
        SingleTokenTestCase{orion::syntax::TokenKind::kStringLiteral,
                            U"\"Hello \\\\ World\"",
                            "StringLiteralWithBackslashEscapedCharacter"},

        // Boolean Literals
        SingleTokenTestCase{orion::syntax::TokenKind::kBooleanLiteral, U"true",
                            "TrueBooleanLiteral"},
        SingleTokenTestCase{orion::syntax::TokenKind::kBooleanLiteral, U"false",
                            "FalseBooleanLiteral"},

        // Integer Literals
        SingleTokenTestCase{orion::syntax::TokenKind::kIntLiteral, U"1337",
                            "IntLiteral"},
        SingleTokenTestCase{orion::syntax::TokenKind::kIntLiteral, U"1337E3",
                            "IntLiteralWithBasicExponent"},
        SingleTokenTestCase{orion::syntax::TokenKind::kIntLiteral, U"1337E+3",
                            "IntLiteralWithPlusExponent"},
        SingleTokenTestCase{orion::syntax::TokenKind::kIntLiteral, U"1337E-3",
                            "IntLiteralWithMinusExponent"},

        // BigDecimal Literals
        SingleTokenTestCase{orion::syntax::TokenKind::kBigDecimalLiteral,
                            U"1337BD", "BigDecimalLiteralUppercaseQuantifier"},
        SingleTokenTestCase{orion::syntax::TokenKind::kBigDecimalLiteral,
                            U"1337bd", "BigDecimalLiteralLowercaseQuantifier"},
        SingleTokenTestCase{orion::syntax::TokenKind::kBigDecimalLiteral,
                            U"1337E3BD",
                            "BigDecimalLiteralWithBasicExponentAndQuantifier"},

        // BigInt Literals
        SingleTokenTestCase{orion::syntax::TokenKind::kBigIntLiteral, U"1337L",
                            "BigIntLiteralUppercaseQuantifier"},
        SingleTokenTestCase{orion::syntax::TokenKind::kBigIntLiteral, U"1337l",
                            "BigIntLiteralLowercaseQuantifier"},
        SingleTokenTestCase{orion::syntax::TokenKind::kBigIntLiteral,
                            U"1337E3L",
                            "BigIntlLiteralWithBasicExponentAndQuantifier"},

        // SmallInt Literals
        SingleTokenTestCase{orion::syntax::TokenKind::kSmallIntLiteral,
                            U"1337S", "SmallIntLiteralUppercaseQuantifier"},
        SingleTokenTestCase{orion::syntax::TokenKind::kSmallIntLiteral,
                            U"1337s", "SmallIntLiteralLowercaseQuantifier"},
        SingleTokenTestCase{orion::syntax::TokenKind::kSmallIntLiteral,
                            U"1337E3S",
                            "SmallIntlLiteralWithBasicExponentAndQuantifier"},

        // TinyInt Literals
        SingleTokenTestCase{orion::syntax::TokenKind::kTinyIntLiteral, U"1337Y",
                            "TinyIntLiteralUppercaseQuantifier"},
        SingleTokenTestCase{orion::syntax::TokenKind::kTinyIntLiteral, U"1337y",
                            "TinyIntLiteralLowercaseQuantifier"},
        SingleTokenTestCase{orion::syntax::TokenKind::kTinyIntLiteral,
                            U"1337E3Y",
                            "TinyIntlLiteralWithBasicExponentAndQuantifier"},

        // Float Literals
        SingleTokenTestCase{orion::syntax::TokenKind::kFloatLiteral, U"3.14",
                            "FloatLiteral"},
        SingleTokenTestCase{orion::syntax::TokenKind::kFloatLiteral, U".314",
                            "FloatLiteralNoLeadingDigit"},

        SingleTokenTestCase{orion::syntax::TokenKind::kFloatLiteral, U"3.14E3",
                            "FloatLiteralWithBasicExponent"},
        SingleTokenTestCase{orion::syntax::TokenKind::kFloatLiteral, U"3.14E+3",
                            "FloatLiteralWithPlusExponent"},
        SingleTokenTestCase{orion::syntax::TokenKind::kFloatLiteral, U"3.14E-3",
                            "FloatLiteralWithMinusExponent"},
        SingleTokenTestCase{orion::syntax::TokenKind::kFloatLiteral, U".314E3",
                            "FloatLiteralNoLeadingDigitWithBasicExponent"},
        SingleTokenTestCase{orion::syntax::TokenKind::kFloatLiteral, U".314E+3",
                            "FloatLiteralNoLeadingDigitWithPlusExponent"},
        SingleTokenTestCase{orion::syntax::TokenKind::kFloatLiteral, U".314E-3",
                            "FloatLiteralNoLeadingDigitWithMinusExponent"},

        SingleTokenTestCase{orion::syntax::TokenKind::kFloatLiteral, U"3.14F",
                            "FloatLiteralUppercaseQuantifier"},
        SingleTokenTestCase{orion::syntax::TokenKind::kFloatLiteral, U"3.14f",
                            "FloatLiteralLowercaseQuantifier"},
        SingleTokenTestCase{orion::syntax::TokenKind::kFloatLiteral, U"3.14E3F",
                            "FloatLiteralWithBasicExponentAndQuantifier"},

        // Double Literals
        SingleTokenTestCase{orion::syntax::TokenKind::kDoubleLit, U"3.14D",
                            "DoubleLiteralUppercaseQuantifier"},
        SingleTokenTestCase{orion::syntax::TokenKind::kDoubleLit, U"3.14d",
                            "DoubleLiteralLowercaseQuantifier"},
        SingleTokenTestCase{orion::syntax::TokenKind::kDoubleLit, U"3.14E3D",
                            "DoubleLiteralWithBasicExponentAndQuantifier"}),
    [](const testing::TestParamInfo<
        SingleTokenParameterizedTestFixture::ParamType>& info) {
      return info.param.test_name;
    });
TEST_P(SingleTokenParameterizedTestFixture, SingleTokens) {
  const SingleTokenTestCase& param = GetParam();
  auto lexer = orion::syntax::Lexer(param.source);
  const std::optional<orion::syntax::Token> expected =
      orion::syntax::BuildToken(param.kind, param.source);
  const std::optional<orion::syntax::Token> actual = lexer.TryNextToken();

  EXPECT_EQ(expected, actual);
}

TEST(LexerTest, MultipleIntLit) {
  const std::u32string utf8 = U"1337 3144";
  auto lexer = orion::syntax::Lexer(utf8);
  const auto expected_1 = orion::syntax::BuildToken(
      orion::syntax::TokenKind::kIntLiteral, 0, 4, U"1337");
  const auto expected_2 = orion::syntax::BuildToken(
      orion::syntax::TokenKind::kWhitespace, 4, 5, U" ");
  const auto expected_3 = orion::syntax::BuildToken(
      orion::syntax::TokenKind::kIntLiteral, 5, 9, U"3144");
  EXPECT_EQ(expected_1, lexer.TryNextToken());
  EXPECT_EQ(expected_2, lexer.TryNextToken());
  EXPECT_EQ(expected_3, lexer.TryNextToken());
}
}  // namespace