#include <gtest/gtest.h>

#include <codecvt>
#include <locale>
#include <optional>
#include <string>

#include "syntax/lexer/lexer.h"
#include "syntax/lexer/span.h"
#include "syntax/lexer/token_kind.h"

namespace orion::syntax {
struct SingleTokenTestCase {
  TokenKind kind;
  std::string source;
  std::string test_name;
};

class SingleTokenParameterizedTestFixture
    : public ::testing::TestWithParam<SingleTokenTestCase> {};

std::optional<Token> BuildToken(const TokenKind kind, const size_t start,
                                const size_t stop, const std::string& source) {
  return std::make_optional(
      Token(static_cast<uint16_t>(kind), Span(start, stop), source));
}

std::optional<Token> BuildToken(const TokenKind kind,
                                const std::string& source) {
  return BuildToken(kind, 0, source.length(), source);
}

INSTANTIATE_TEST_SUITE_P(
    LexerTest, SingleTokenParameterizedTestFixture,
    ::testing::Values(
        // Operators
        SingleTokenTestCase{TokenKind::kPlus, "+", "Plus"},
        SingleTokenTestCase{TokenKind::kMinus, "-", "Minus"},
        SingleTokenTestCase{TokenKind::kAsterisk, "*", "Asterisk"},
        SingleTokenTestCase{TokenKind::kSlash, "/", "Slash"},
        SingleTokenTestCase{TokenKind::kPercent, "%", "Percent"},

        // Identifiers
        SingleTokenTestCase{TokenKind::kIdentifier, "myIdent", "Identifier"},
        SingleTokenTestCase{TokenKind::kIdentifier, "myIdent123",
                            "IdentifierWithDigits"},
        SingleTokenTestCase{TokenKind::kIdentifier, "🍕", "UnicodeIdentifier"},

        // Integer Literals
        SingleTokenTestCase{TokenKind::kIntLiteral, "1337", "IntLiteral"},

        // Float Literals
        SingleTokenTestCase{TokenKind::kFloatLiteral, "3.14", "FloatLiteral"},
        SingleTokenTestCase{TokenKind::kFloatLiteral, ".314",
                            "FloatLiteralNoLeadingDigit"},
        SingleTokenTestCase{TokenKind::kFloatLiteral, "3.14f",
                            "FloatLiteralLowercaseQuantifier"},
        SingleTokenTestCase{TokenKind::kFloatLiteral, "3.14F",
                            "FloatLiteralUppercaseQuantifier"}),
    [](const testing::TestParamInfo<
        SingleTokenParameterizedTestFixture::ParamType>& info) {
      return info.param.test_name;
    });
TEST_P(SingleTokenParameterizedTestFixture, SingleTokens) {
  const SingleTokenTestCase& param = GetParam();
  auto lexer = Lexer(param.source);
  const std::optional<Token> expected = BuildToken(param.kind, param.source);
  const std::optional<Token> actual = lexer.TryNextToken();
  EXPECT_EQ(expected, actual);
}

TEST(LexerTest, MultipleIntLit) {
  auto lexer = Lexer("1337 3144");
  const auto expected_1 = BuildToken(TokenKind::kIntLiteral, 0, 4, "1337");
  const auto expected_2 = BuildToken(TokenKind::kWhitespace, 4, 5, " ");
  const auto expected_3 = BuildToken(TokenKind::kIntLiteral, 5, 9, "3144");
  EXPECT_EQ(expected_1, lexer.TryNextToken());
  EXPECT_EQ(expected_2, lexer.TryNextToken());
  EXPECT_EQ(expected_3, lexer.TryNextToken());
}
}  // namespace orion::syntax