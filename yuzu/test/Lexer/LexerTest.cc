#include "yuzu/Lexer/Lexer.h"
#include "yuzu/Lexer/Token.h"
#include "yuzu/Lexer/TokenKind.h"

#include "gtest/gtest.h"

#include <optional>
#include <string>
#include <vector>

namespace {
using yuzu::lexer::Lexer;
using yuzu::lexer::Token;
using yuzu::lexer::TokenKind;

struct SingleTokenLexerParam {
  const std::u32string source;
  const Token expected;
};

struct MultiTokenLexerParam {
  const std::u32string source;
  const std::vector<Token> expected;
};

class LexerSingleTokenTest
    : public ::testing::TestWithParam<SingleTokenLexerParam> {};

class LexerMultiTokenTest
    : public ::testing::TestWithParam<MultiTokenLexerParam> {};

TEST_P(LexerSingleTokenTest, GetNextToken) {
  const SingleTokenLexerParam &param = GetParam();

  auto lexer = Lexer(param.source);
  const std::optional<Token> actual = lexer.getNextToken();
  EXPECT_EQ(param.expected, actual);
  EXPECT_EQ(std::nullopt, lexer.getNextToken());
}

TEST_P(LexerMultiTokenTest, GetNextToken) {
  const MultiTokenLexerParam &param = GetParam();

  auto lexer = Lexer(param.source);
  for (const auto &expected : param.expected) {
    EXPECT_EQ(expected, lexer.getNextToken());
  }

  EXPECT_EQ(std::nullopt, lexer.getNextToken());
}

INSTANTIATE_TEST_SUITE_P(
    GetSingleTokenParams, LexerSingleTokenTest,
    ::testing::Values(
        SingleTokenLexerParam{U"=", Token(TokenKind::Equals, U"=", 0, 1)},
        SingleTokenLexerParam{U"-", Token(TokenKind::Minus, U"-", 0, 1)},
        SingleTokenLexerParam{U"+", Token(TokenKind::Plus, U"+", 0, 1)},
        SingleTokenLexerParam{U"/", Token(TokenKind::Slash, U"/", 0, 1)},
        SingleTokenLexerParam{U"*", Token(TokenKind::Star, U"*", 0, 1)},
        SingleTokenLexerParam{U"123", Token(TokenKind::Number, U"123", 0, 3)},
        SingleTokenLexerParam{U"// hello",
                              Token(TokenKind::Comment, U"// hello", 0, 8)},
        SingleTokenLexerParam{U"my_1d3nt",
                              Token(TokenKind::Ident, U"my_1d3nt", 0, 8)}));

INSTANTIATE_TEST_SUITE_P(
    GetMultiTokenParams, LexerMultiTokenTest,
    ::testing::Values(
        MultiTokenLexerParam{U"=+",
                             std::vector<Token>{
                                 Token(TokenKind::Equals, U"=", 0, 1),
                                 Token(TokenKind::Plus, U"+", 1, 2),
                             }},
        MultiTokenLexerParam{
            U"my_1d3nt 123 other_1d3nt 456",
            std::vector<Token>{
                Token(TokenKind::Ident, U"my_1d3nt", 0, 8),
                Token(TokenKind::Space, U" ", 8, 9),
                Token(TokenKind::Number, U"123", 9, 12),
                Token(TokenKind::Space, U" ", 12, 13),
                Token(TokenKind::Ident, U"other_1d3nt", 13, 24),
                Token(TokenKind::Space, U" ", 24, 25),
                Token(TokenKind::Number, U"456", 25, 28),
            }}));
}; // namespace
