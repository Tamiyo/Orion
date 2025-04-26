#include <gtest/gtest.h>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "lang/lexer/lexer.h"
#include "lang/lexer/token_kind.h"
#include "syntax/lexer/span.h"
#include "syntax/lexer/token.h"

namespace yuzu::lang {
std::optional<syntax::Token<TokenKind>> BuildToken(
    const TokenKind kind, const size_t start, const size_t stop,
    const std::u32string& source) {
  return std::make_optional(
      syntax::Token(kind, syntax::Span(start, stop), std::move(source)));
}

std::optional<syntax::Token<TokenKind>> BuildToken(
    const TokenKind kind, const std::u32string& source) {
  return BuildToken(kind, 0, source.length(), std::move(source));
}
}  // namespace yuzu::lang

namespace {
using yuzu::lang::Lexer;
using yuzu::lang::TokenKind;
using yuzu::syntax::Token;

struct SingleTokenTestCase {
  TokenKind kind;
  std::u32string source;
  std::string test_name;
};

class SingleTokenParameterizedTestFixture
    : public testing::TestWithParam<SingleTokenTestCase> {};

INSTANTIATE_TEST_SUITE_P(
    LexerTest, SingleTokenParameterizedTestFixture,
    ::testing::Values(
        // Keywords

        // Punctuation
        SingleTokenTestCase{TokenKind::kLeftParen, U"(", "LeftParen"},
        SingleTokenTestCase{TokenKind::kRightParen, U")", "RightParen"},
        SingleTokenTestCase{TokenKind::kLeftSquare, U"[", "LeftSquare"},
        SingleTokenTestCase{TokenKind::kRightSquare, U"]", "RightSquare"},

        // Operators
        SingleTokenTestCase{TokenKind::kPlus, U"+", "Plus"},
        SingleTokenTestCase{TokenKind::kMinus, U"-", "Minus"},
        SingleTokenTestCase{TokenKind::kAsterisk, U"*", "Asterisk"},
        SingleTokenTestCase{TokenKind::kSlash, U"/", "Slash"},
        SingleTokenTestCase{TokenKind::kPercent, U"%", "Percent"},

        // Identifiers
        SingleTokenTestCase{TokenKind::kUnquotedIdent, U"_", "IdentUnderscore"},
        SingleTokenTestCase{TokenKind::kUnquotedIdent, U"_a",
                            "IdentUnderscoreletter"},
        SingleTokenTestCase{TokenKind::kUnquotedIdent, U"_1",
                            "IdentUnderscoreDigit"},
        SingleTokenTestCase{TokenKind::kUnquotedIdent, U"_a1",
                            "IdentUnderscoreLetterDigit"},
        SingleTokenTestCase{TokenKind::kUnquotedIdent, U"_1a",
                            "IdentUnderscoreDigitLetter"},
        SingleTokenTestCase{TokenKind::kUnquotedIdent, U"h", "IdentShort"},
        SingleTokenTestCase{TokenKind::kUnquotedIdent, U"hhhhh", "IdentLong"},
        SingleTokenTestCase{TokenKind::kUnquotedIdent, U"h1",
                            "IdentWithDigitsShort"},
        SingleTokenTestCase{TokenKind::kUnquotedIdent, U"hg314141gas151fafsg1",
                            "IdentWithDigitsLong"},
        SingleTokenTestCase{TokenKind::kUnquotedIdent, U"_AA_BB_112abG_51",
                            "IdentMixed"},

        // Quoted Identifiers
        SingleTokenTestCase{TokenKind::kQuotedIdent, U"``",
                            "QuotedIdentNoChars"},
        SingleTokenTestCase{TokenKind::kQuotedIdent, U"` `",
                            "QuotedIdentWithSpace"},
        SingleTokenTestCase{TokenKind::kQuotedIdent, U"` hello``world `",
                            "QuotedIdentWithDoubleBacktick"},
        SingleTokenTestCase{TokenKind::kQuotedIdent, U"`hello_world 123`",
                            "QuotedIdentWithChars"},
        SingleTokenTestCase{TokenKind::kQuotedIdent, U"`伂告伒伄伌伜`",
                            "QuotedIdentWithUnicodeChars"},

        // Unicode Idents
        SingleTokenTestCase{TokenKind::kUnquotedIdent, U"🍕", "UnicodeIdent"},
        SingleTokenTestCase{TokenKind::kUnquotedIdent, U"伂告伒伄伌伜",
                            "UnicodeIdentMultipleChars"},

        // String Literals
        SingleTokenTestCase{TokenKind::kStringLiteral, U"\"Hello World\"",
                            "StringLiteral"},
        SingleTokenTestCase{TokenKind::kStringLiteral, U"\"Hello \\t World\"",
                            "StringLiteralWithTabEscapedCharacter"},
        SingleTokenTestCase{TokenKind::kStringLiteral, U"\"Hello \\b World\"",
                            "StringLiteralWithBackspaceEscapedCharacter"},
        SingleTokenTestCase{TokenKind::kStringLiteral, U"\"Hello \\n World\"",
                            "StringLiteralWithNewlineEscapedCharacter"},
        SingleTokenTestCase{TokenKind::kStringLiteral, U"\"Hello \\r World\"",
                            "StringLiteralWithCarriageReturnEscapedCharacter"},
        SingleTokenTestCase{TokenKind::kStringLiteral, U"\"Hello \\f World\"",
                            "StringLiteralWithFormFeedEscapedCharacter"},
        SingleTokenTestCase{TokenKind::kStringLiteral, U"\"Hello \\' World\"",
                            "StringLiteralWithQuoteEscapedCharacter"},
        SingleTokenTestCase{TokenKind::kStringLiteral, U"\"Hello \\\" World\"",
                            "StringLiteralWithDoubleQuoteEscapedCharacter"},
        SingleTokenTestCase{TokenKind::kStringLiteral, U"\"Hello \\\\ World\"",
                            "StringLiteralWithBackslashEscapedCharacter"},

        // Boolean Literals
        SingleTokenTestCase{TokenKind::kBooleanLiteral, U"true",
                            "TrueBooleanLiteral"},
        SingleTokenTestCase{TokenKind::kBooleanLiteral, U"false",
                            "FalseBooleanLiteral"},

        // Integer Literals
        SingleTokenTestCase{TokenKind::kIntLiteral, U"1337", "IntLiteral"},
        SingleTokenTestCase{TokenKind::kIntLiteral, U"1337E3",
                            "IntLiteralWithBasicExponent"},
        SingleTokenTestCase{TokenKind::kIntLiteral, U"1337E+3",
                            "IntLiteralWithPlusExponent"},
        SingleTokenTestCase{TokenKind::kIntLiteral, U"1337E-3",
                            "IntLiteralWithMinusExponent"},

        // BigDecimal Literals
        SingleTokenTestCase{TokenKind::kBigDecimalLiteral, U"1337BD",
                            "BigDecimalLiteralUppercaseQuantifier"},
        SingleTokenTestCase{TokenKind::kBigDecimalLiteral, U"1337bd",
                            "BigDecimalLiteralLowercaseQuantifier"},
        SingleTokenTestCase{TokenKind::kBigDecimalLiteral, U"1337E3BD",
                            "BigDecimalLiteralWithBasicExponentAndQuantifier"},

        // BigInt Literals
        SingleTokenTestCase{TokenKind::kBigIntLiteral, U"1337L",
                            "BigIntLiteralUppercaseQuantifier"},
        SingleTokenTestCase{TokenKind::kBigIntLiteral, U"1337l",
                            "BigIntLiteralLowercaseQuantifier"},
        SingleTokenTestCase{TokenKind::kBigIntLiteral, U"1337E3L",
                            "BigIntlLiteralWithBasicExponentAndQuantifier"},

        // SmallInt Literals
        SingleTokenTestCase{TokenKind::kSmallIntLiteral, U"1337S",
                            "SmallIntLiteralUppercaseQuantifier"},
        SingleTokenTestCase{TokenKind::kSmallIntLiteral, U"1337s",
                            "SmallIntLiteralLowercaseQuantifier"},
        SingleTokenTestCase{TokenKind::kSmallIntLiteral, U"1337E3S",
                            "SmallIntlLiteralWithBasicExponentAndQuantifier"},

        // TinyInt Literals
        SingleTokenTestCase{TokenKind::kTinyIntLiteral, U"1337Y",
                            "TinyIntLiteralUppercaseQuantifier"},
        SingleTokenTestCase{TokenKind::kTinyIntLiteral, U"1337y",
                            "TinyIntLiteralLowercaseQuantifier"},
        SingleTokenTestCase{TokenKind::kTinyIntLiteral, U"1337E3Y",
                            "TinyIntlLiteralWithBasicExponentAndQuantifier"},

        // Float Literals
        SingleTokenTestCase{TokenKind::kFloatLiteral, U"3.14", "FloatLiteral"},
        SingleTokenTestCase{TokenKind::kFloatLiteral, U".314",
                            "FloatLiteralNoLeadingDigit"},

        SingleTokenTestCase{TokenKind::kFloatLiteral, U"3.14E3",
                            "FloatLiteralWithBasicExponent"},
        SingleTokenTestCase{TokenKind::kFloatLiteral, U"3.14E+3",
                            "FloatLiteralWithPlusExponent"},
        SingleTokenTestCase{TokenKind::kFloatLiteral, U"3.14E-3",
                            "FloatLiteralWithMinusExponent"},
        SingleTokenTestCase{TokenKind::kFloatLiteral, U".314E3",
                            "FloatLiteralNoLeadingDigitWithBasicExponent"},
        SingleTokenTestCase{TokenKind::kFloatLiteral, U".314E+3",
                            "FloatLiteralNoLeadingDigitWithPlusExponent"},
        SingleTokenTestCase{TokenKind::kFloatLiteral, U".314E-3",
                            "FloatLiteralNoLeadingDigitWithMinusExponent"},

        SingleTokenTestCase{TokenKind::kFloatLiteral, U"3.14F",
                            "FloatLiteralUppercaseQuantifier"},
        SingleTokenTestCase{TokenKind::kFloatLiteral, U"3.14f",
                            "FloatLiteralLowercaseQuantifier"},
        SingleTokenTestCase{TokenKind::kFloatLiteral, U"3.14E3F",
                            "FloatLiteralWithBasicExponentAndQuantifier"},

        // Double Literals
        SingleTokenTestCase{TokenKind::kDoubleLit, U"3.14D",
                            "DoubleLiteralUppercaseQuantifier"},
        SingleTokenTestCase{TokenKind::kDoubleLit, U"3.14d",
                            "DoubleLiteralLowercaseQuantifier"},
        SingleTokenTestCase{TokenKind::kDoubleLit, U"3.14E3D",
                            "DoubleLiteralWithBasicExponentAndQuantifier"}),
    [](const testing::TestParamInfo<
        SingleTokenParameterizedTestFixture::ParamType>& info) {
      return info.param.test_name;
    });
TEST_P(SingleTokenParameterizedTestFixture, SingleTokens) {
  const SingleTokenTestCase& param = GetParam();
  auto lexer = Lexer(param.source);
  const std::optional<Token<TokenKind>> expected =
      BuildToken(param.kind, param.source);

  const std::vector<Token<TokenKind>> tokens = lexer.Tokenize();
  ASSERT_EQ(1, tokens.size());

  const Token<TokenKind>& actual = tokens.at(0);
  EXPECT_EQ(expected, actual);
}

TEST(LexerTest, MultipleIntLit) {
  const std::u32string_view utf8 = U"1337 3144";
  auto lexer = Lexer(utf8);
  const auto expected_1 = BuildToken(TokenKind::kIntLiteral, 0, 4, U"1337");
  const auto expected_2 = BuildToken(TokenKind::kWhitespace, 4, 5, U" ");
  const auto expected_3 = BuildToken(TokenKind::kIntLiteral, 5, 9, U"3144");

  const std::vector<Token<TokenKind>> tokens = lexer.Tokenize();
  ASSERT_EQ(3, tokens.size());

  EXPECT_EQ(expected_1, tokens.at(0));
  EXPECT_EQ(expected_2, tokens.at(1));
  EXPECT_EQ(expected_3, tokens.at(2));
}
}  // namespace
