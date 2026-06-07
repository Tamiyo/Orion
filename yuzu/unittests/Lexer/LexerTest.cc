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
    Symbols, LexerTokenTest,
    ::testing::Values(
        LexerParam{U"=", std::vector<Token>{Token(TokenKind::Eq, U"=", 0, 1)}},
        LexerParam{U"+",
                   std::vector<Token>{Token(TokenKind::Plus, U"+", 0, 1)}},
        LexerParam{U"-",
                   std::vector<Token>{Token(TokenKind::Minus, U"-", 0, 1)}},
        LexerParam{U"*",
                   std::vector<Token>{Token(TokenKind::Star, U"*", 0, 1)}},
        LexerParam{U"**",
                   std::vector<Token>{Token(TokenKind::Pow, U"**", 0, 2)}},
        LexerParam{U"/",
                   std::vector<Token>{Token(TokenKind::Slash, U"/", 0, 1)}},
        LexerParam{U"(",
                   std::vector<Token>{Token(TokenKind::LeftParen, U"(", 0, 1)}},
        LexerParam{U")", std::vector<Token>{
                             Token(TokenKind::RightParen, U")", 0, 1)}}));

INSTANTIATE_TEST_SUITE_P(
    Comparison, LexerTokenTest,
    ::testing::Values(
        LexerParam{U"==",
                   std::vector<Token>{Token(TokenKind::EqEq, U"==", 0, 2)}},
        LexerParam{U"!=",
                   std::vector<Token>{Token(TokenKind::Neq, U"!=", 0, 2)}},
        LexerParam{U"<", std::vector<Token>{Token(TokenKind::Lt, U"<", 0, 1)}},
        LexerParam{U"<=",
                   std::vector<Token>{Token(TokenKind::Lte, U"<=", 0, 2)}},
        LexerParam{U">", std::vector<Token>{Token(TokenKind::Gt, U">", 0, 1)}},
        LexerParam{U">=",
                   std::vector<Token>{Token(TokenKind::Gte, U">=", 0, 2)}}));

INSTANTIATE_TEST_SUITE_P(
    Bitwise, LexerTokenTest,
    ::testing::Values(
        LexerParam{U"<<", std::vector<Token>{Token(TokenKind::ShiftLeft, U"<<",
                                                   0, 2)}},
        LexerParam{U">>", std::vector<Token>{
                              Token(TokenKind::ShiftRight, U">>", 0, 2)}}));

INSTANTIATE_TEST_SUITE_P(
    Delimiters, LexerTokenTest,
    ::testing::Values(
        LexerParam{U"[", std::vector<Token>{Token(TokenKind::LeftBracket, U"[",
                                                  0, 1)}},
        LexerParam{U"]", std::vector<Token>{Token(TokenKind::RightBracket, U"]",
                                                  0, 1)}},
        LexerParam{U"{",
                   std::vector<Token>{Token(TokenKind::LeftCurly, U"{", 0, 1)}},
        LexerParam{
            U"}", std::vector<Token>{Token(TokenKind::RightCurly, U"}", 0, 1)}},
        LexerParam{U",",
                   std::vector<Token>{Token(TokenKind::Comma, U",", 0, 1)}},
        LexerParam{U":",
                   std::vector<Token>{Token(TokenKind::Colon, U":", 0, 1)}}));

INSTANTIATE_TEST_SUITE_P(
    Arrow, LexerTokenTest,
    ::testing::Values(
        LexerParam{U"->",
                   std::vector<Token>{Token(TokenKind::Arrow, U"->", 0, 2)}},
        // A lone `-` stays `Minus`; only `->` munches into an `Arrow`.
        LexerParam{U"-",
                   std::vector<Token>{Token(TokenKind::Minus, U"-", 0, 1)}},
        // `-` then a space is `Minus` then `Gt`, not an arrow.
        LexerParam{U"- >",
                   std::vector<Token>{Token(TokenKind::Minus, U"-", 0, 1),
                                      Token(TokenKind::Space, U" ", 1, 2),
                                      Token(TokenKind::Gt, U">", 2, 3)}}));

INSTANTIATE_TEST_SUITE_P(
    Integers, LexerTokenTest,
    ::testing::Values(
        LexerParam{U"0", std::vector<Token>{Token(TokenKind::IntegerLiteral,
                                                  U"0", 0, 1)}},
        LexerParam{U"123", std::vector<Token>{Token(TokenKind::IntegerLiteral,
                                                    U"123", 0, 3)}},
        LexerParam{U"1_000_000",
                   std::vector<Token>{
                       Token(TokenKind::IntegerLiteral, U"1_000_000", 0, 9)}}));

INSTANTIATE_TEST_SUITE_P(
    Floats, LexerTokenTest,
    ::testing::Values(
        LexerParam{U"3.14", std::vector<Token>{Token(TokenKind::FloatLiteral,
                                                     U"3.14", 0, 4)}},
        LexerParam{U"0.0", std::vector<Token>{Token(TokenKind::FloatLiteral,
                                                    U"0.0", 0, 3)}},
        // Trailing dot — the integer part + `.` are consumed even when no
        // fractional digits follow; the lexer still classifies it as a
        // float so the grammar can decide what to do with `2.`.
        LexerParam{U"2.", std::vector<Token>{Token(TokenKind::FloatLiteral,
                                                   U"2.", 0, 2)}},
        // Leading-dot float: dot followed by digits is one token.
        LexerParam{U".5", std::vector<Token>{Token(TokenKind::FloatLiteral,
                                                   U".5", 0, 2)}},
        LexerParam{U"1_000.500_5",
                   std::vector<Token>{
                       Token(TokenKind::FloatLiteral, U"1_000.500_5", 0, 11)}},
        // Exponent on an otherwise-integer literal still produces a
        // float (since `1e5` has a non-integer value).
        LexerParam{U"1e5", std::vector<Token>{Token(TokenKind::FloatLiteral,
                                                    U"1e5", 0, 3)}},
        LexerParam{U"1E5", std::vector<Token>{Token(TokenKind::FloatLiteral,
                                                    U"1E5", 0, 3)}},
        // Exponent sign (`+`/`-`) is part of the float token, not a
        // separate operator.
        LexerParam{U"1e-5", std::vector<Token>{Token(TokenKind::FloatLiteral,
                                                     U"1e-5", 0, 4)}},
        LexerParam{U"1e+10", std::vector<Token>{Token(TokenKind::FloatLiteral,
                                                      U"1e+10", 0, 5)}},
        LexerParam{U"1.5e10", std::vector<Token>{Token(TokenKind::FloatLiteral,
                                                       U"1.5e10", 0, 6)}},
        LexerParam{U".5e-3", std::vector<Token>{Token(TokenKind::FloatLiteral,
                                                      U".5e-3", 0, 5)}},
        // `1e` with no digits after the `e` is NOT a valid exponent; the
        // lexer backs off and emits the integer followed by an `e` ident.
        LexerParam{U"1e", std::vector<Token>{
                              Token(TokenKind::IntegerLiteral, U"1", 0, 1),
                              Token(TokenKind::Identifier, U"e", 1, 2)}}));

INSTANTIATE_TEST_SUITE_P(
    Strings, LexerTokenTest,
    ::testing::Values(
        LexerParam{U"\"\"", std::vector<Token>{Token(TokenKind::StringLiteral,
                                                     U"\"\"", 0, 2)}},
        LexerParam{U"\"hello\"",
                   std::vector<Token>{
                       Token(TokenKind::StringLiteral, U"\"hello\"", 0, 7)}},
        // Escapes don't terminate the string.
        LexerParam{U"\"a\\\"b\"",
                   std::vector<Token>{
                       Token(TokenKind::StringLiteral, U"\"a\\\"b\"", 0, 6)}},
        LexerParam{U"\"line\\nbreak\"",
                   std::vector<Token>{Token(TokenKind::StringLiteral,
                                            U"\"line\\nbreak\"", 0, 13)}},
        // Unterminated string (EOF before closing quote) becomes Error.
        LexerParam{U"\"oops", std::vector<Token>{
                                  Token(TokenKind::Error, U"\"oops", 0, 5)}}));

INSTANTIATE_TEST_SUITE_P(
    RawStrings, LexerTokenTest,
    ::testing::Values(
        LexerParam{U"r\"\"",
                   std::vector<Token>{
                       Token(TokenKind::RawStringLiteral, U"r\"\"", 0, 3)}},
        LexerParam{U"r\"hello\"",
                   std::vector<Token>{Token(TokenKind::RawStringLiteral,
                                            U"r\"hello\"", 0, 8)}},
        // Backslashes inside a raw string are literal — `\\\"` does NOT
        // escape the closing quote, so the string ends at the first `"`.
        LexerParam{U"r\"a\\\"b\"",
                   std::vector<Token>{
                       Token(TokenKind::RawStringLiteral, U"r\"a\\\"", 0, 5),
                       Token(TokenKind::Identifier, U"b", 5, 6),
                       Token(TokenKind::Error, U"\"", 6, 7)}}));

INSTANTIATE_TEST_SUITE_P(
    HexLiterals, LexerTokenTest,
    ::testing::Values(
        LexerParam{U"0x1A", std::vector<Token>{Token(TokenKind::HexLiteral,
                                                     U"0x1A", 0, 4)}},
        LexerParam{U"0XFF", std::vector<Token>{Token(TokenKind::HexLiteral,
                                                     U"0XFF", 0, 4)}},
        LexerParam{U"0x1_FF_AB", std::vector<Token>{Token(TokenKind::HexLiteral,
                                                          U"0x1_FF_AB", 0, 9)}},
        // `0x` with no hex digits is an error token spanning the prefix.
        LexerParam{U"0x",
                   std::vector<Token>{Token(TokenKind::Error, U"0x", 0, 2)}}));

INSTANTIATE_TEST_SUITE_P(
    BinaryLiterals, LexerTokenTest,
    ::testing::Values(
        LexerParam{U"0b0", std::vector<Token>{Token(TokenKind::BinaryLiteral,
                                                    U"0b0", 0, 3)}},
        LexerParam{U"0b101", std::vector<Token>{Token(TokenKind::BinaryLiteral,
                                                      U"0b101", 0, 5)}},
        LexerParam{U"0B1_0_1",
                   std::vector<Token>{
                       Token(TokenKind::BinaryLiteral, U"0B1_0_1", 0, 7)}},
        // `0b` with no binary digits is an error token spanning the prefix.
        LexerParam{U"0b",
                   std::vector<Token>{Token(TokenKind::Error, U"0b", 0, 2)}}));

INSTANTIATE_TEST_SUITE_P(
    Booleans, LexerTokenTest,
    ::testing::Values(
        LexerParam{U"true", std::vector<Token>{Token(TokenKind::BooleanLiteral,
                                                     U"true", 0, 4)}},
        LexerParam{U"false", std::vector<Token>{Token(TokenKind::BooleanLiteral,
                                                      U"false", 0, 5)}}));

INSTANTIATE_TEST_SUITE_P(
    Identifiers, LexerTokenTest,
    ::testing::Values(
        LexerParam{U"my_1d3nt", std::vector<Token>{Token(TokenKind::Identifier,
                                                         U"my_1d3nt", 0, 8)}},
        // `true` / `false` / `let` / `mut` are keywords; anything else
        // shaped like an identifier becomes `Ident`.
        LexerParam{U"truer", std::vector<Token>{Token(TokenKind::Identifier,
                                                      U"truer", 0, 5)}}));

INSTANTIATE_TEST_SUITE_P(
    Keywords, LexerTokenTest,
    ::testing::Values(
        LexerParam{U"and",
                   std::vector<Token>{Token(TokenKind::AndKw, U"and", 0, 3)}},
        LexerParam{U"in",
                   std::vector<Token>{Token(TokenKind::InKw, U"in", 0, 2)}},
        LexerParam{U"let",
                   std::vector<Token>{Token(TokenKind::LetKw, U"let", 0, 3)}},
        LexerParam{U"mut",
                   std::vector<Token>{Token(TokenKind::MutKw, U"mut", 0, 3)}},
        LexerParam{U"not",
                   std::vector<Token>{Token(TokenKind::NotKw, U"not", 0, 3)}},
        LexerParam{U"or",
                   std::vector<Token>{Token(TokenKind::OrKw, U"or", 0, 2)}},
        // A keyword that is only a prefix of a longer identifier stays an
        // identifier; the whole word is lexed before keyword matching.
        LexerParam{U"input", std::vector<Token>{Token(TokenKind::Identifier,
                                                      U"input", 0, 5)}},
        LexerParam{U"android", std::vector<Token>{Token(TokenKind::Identifier,
                                                        U"android", 0, 7)}},
        LexerParam{U"order", std::vector<Token>{Token(TokenKind::Identifier,
                                                      U"order", 0, 5)}},
        LexerParam{U"nothing", std::vector<Token>{Token(TokenKind::Identifier,
                                                        U"nothing", 0, 7)}}));

INSTANTIATE_TEST_SUITE_P(
    Trivia, LexerTokenTest,
    ::testing::Values(
        LexerParam{U"// hello", std::vector<Token>{Token(TokenKind::Comment,
                                                         U"// hello", 0, 8)}},
        LexerParam{U" ",
                   std::vector<Token>{Token(TokenKind::Space, U" ", 0, 1)}},
        LexerParam{U"\n", std::vector<Token>{
                              Token(TokenKind::Newline, U"\n", 0, 1)}}));

INSTANTIATE_TEST_SUITE_P(
    MultiToken, LexerTokenTest,
    ::testing::Values(
        LexerParam{U"=+",
                   std::vector<Token>{Token(TokenKind::Eq, U"=", 0, 1),
                                      Token(TokenKind::Plus, U"+", 1, 2)}},
        LexerParam{
            U"let x = 42",
            std::vector<Token>{Token(TokenKind::LetKw, U"let", 0, 3),
                               Token(TokenKind::Space, U" ", 3, 4),
                               Token(TokenKind::Identifier, U"x", 4, 5),
                               Token(TokenKind::Space, U" ", 5, 6),
                               Token(TokenKind::Eq, U"=", 6, 7),
                               Token(TokenKind::Space, U" ", 7, 8),
                               Token(TokenKind::IntegerLiteral, U"42", 8, 10)}},
        LexerParam{
            U"(1 + 0x2)",
            std::vector<Token>{Token(TokenKind::LeftParen, U"(", 0, 1),
                               Token(TokenKind::IntegerLiteral, U"1", 1, 2),
                               Token(TokenKind::Space, U" ", 2, 3),
                               Token(TokenKind::Plus, U"+", 3, 4),
                               Token(TokenKind::Space, U" ", 4, 5),
                               Token(TokenKind::HexLiteral, U"0x2", 5, 8),
                               Token(TokenKind::RightParen, U")", 8, 9)}},
        // `<<` is matched before `<=`, so the trailing `=` is its own token.
        LexerParam{U"<<=",
                   std::vector<Token>{Token(TokenKind::ShiftLeft, U"<<", 0, 2),
                                      Token(TokenKind::Eq, U"=", 2, 3)}},
        LexerParam{U">>=",
                   std::vector<Token>{Token(TokenKind::ShiftRight, U">>", 0, 2),
                                      Token(TokenKind::Eq, U"=", 2, 3)}},
        // `<=` is matched maximally, leaving the extra `=` separate.
        LexerParam{U"<==",
                   std::vector<Token>{Token(TokenKind::Lte, U"<=", 0, 2),
                                      Token(TokenKind::Eq, U"=", 2, 3)}},
        // `**` is matched maximally; a third `*` is its own `Star`.
        LexerParam{U"***",
                   std::vector<Token>{Token(TokenKind::Pow, U"**", 0, 2),
                                      Token(TokenKind::Star, U"*", 2, 3)}},
        // `==` is matched maximally; a third `=` is a lone `Eq`.
        LexerParam{U"===",
                   std::vector<Token>{Token(TokenKind::EqEq, U"==", 0, 2),
                                      Token(TokenKind::Eq, U"=", 2, 3)}},
        // `!=` consumes both characters; the trailing `=` is separate.
        LexerParam{U"!==",
                   std::vector<Token>{Token(TokenKind::Neq, U"!=", 0, 2),
                                      Token(TokenKind::Eq, U"=", 2, 3)}}));

INSTANTIATE_TEST_SUITE_P(
    Unknown, LexerTokenTest,
    ::testing::Values(
        // Unrecognized characters produce a single-character `Error`
        // token so the lexer keeps making progress instead of dropping
        // the rest of the input.
        LexerParam{U"@",
                   std::vector<Token>{Token(TokenKind::Error, U"@", 0, 1)}},
        // A lone `!` is not an operator (negation is the `not` keyword), so
        // it lexes as a single-character `Error`.
        LexerParam{U"!",
                   std::vector<Token>{Token(TokenKind::Error, U"!", 0, 1)}}));

} // namespace
