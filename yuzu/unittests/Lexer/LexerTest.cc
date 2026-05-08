#include "yuzu/Lexer/Lexer.h"
#include "yuzu/Lexer/Token.h"
#include "yuzu/Lexer/TokenKind.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace {
using yuzu::lexer::Lexer;
using yuzu::lexer::Token;
using yuzu::lexer::TokenKind;

struct LexerParam {
  const std::u32string source;
  const std::vector<Token> expected;
};

class LexerTokenTest : public ::testing::TestWithParam<LexerParam> {};

TEST_P(LexerTokenTest, GetTokens) {
  const LexerParam &param = GetParam();

  auto lexer = Lexer(param.source);
  EXPECT_EQ(param.expected, lexer.getTokens());
}

INSTANTIATE_TEST_SUITE_P(
    GetLexerParams, LexerTokenTest,
    ::testing::Values(
        LexerParam{U"=",
                   std::vector<Token>{
                       Token(TokenKind::Equals, U"=", 0, 1),
                   }},
        LexerParam{U"-",
                   std::vector<Token>{
                       Token(TokenKind::Minus, U"-", 0, 1),
                   }},
        LexerParam{U"+",
                   std::vector<Token>{
                       Token(TokenKind::Plus, U"+", 0, 1),
                   }},
        LexerParam{U"/",
                   std::vector<Token>{
                       Token(TokenKind::Slash, U"/", 0, 1),
                   }},
        LexerParam{U"*",
                   std::vector<Token>{
                       Token(TokenKind::Star, U"*", 0, 1),
                   }},
        LexerParam{U"123",
                   std::vector<Token>{
                       Token(TokenKind::Number, U"123", 0, 3),
                   }},
        LexerParam{U"// hello",
                   std::vector<Token>{
                       Token(TokenKind::Comment, U"// hello", 0, 8),
                   }},
        LexerParam{
            U"my_1d3nt",
            std::vector<Token>{Token(TokenKind::Ident, U"my_1d3nt", 0, 8)},
        },
        LexerParam{
            U"let",
            std::vector<Token>{Token(TokenKind::LetKw, U"let", 0, 3)},
        },
        LexerParam{
            U"mut",
            std::vector<Token>{Token(TokenKind::MutKw, U"mut", 0, 3)},
        },
        LexerParam{U"=+",
                   std::vector<Token>{
                       Token(TokenKind::Equals, U"=", 0, 1),
                       Token(TokenKind::Plus, U"+", 1, 2),
                   }},
        LexerParam{U"my_1d3nt 123 other_1d3nt 456",
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
