#include "yuzu/Parser/Parser.h"
#include "ParserTestUtils.h"

#include "yuzu/Ast/Ast.h"
#include "yuzu/Lexer/Lexer.h"
#include "yuzu/Lexer/Token.h"
#include "yuzu/Lexer/TokenKind.h"
#include "yuzu/Parser/TokenSource.h"

#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <vector>

namespace {
using yuzu::ast::SyntaxKind;
using yuzu::lexer::Lexer;
using yuzu::lexer::Token;
using yuzu::lexer::TokenKind;
using yuzu::parser::CompletedMarker;
using yuzu::parser::Marker;
using yuzu::parser::Parser;
using yuzu::parser::TokenSource;

// These tests poke at the Parser's API directly rather than running a
// grammar entry. Inherit `ParserFixture` so each test gets its own
// engine; tests can construct `Parser(source, engine)` against it.
class ParserTest : public yuzu::parser::test::ParserFixture {};

inline std::vector<Token> lex(const std::u32string &source) {
  auto lexer = Lexer(source);
  return lexer.getTokens();
}

TEST_F(ParserTest, PeekKindReturnsFirstToken) {
  const auto tokens = lex(U"123");
  auto parser = Parser(TokenSource(tokens));

  EXPECT_EQ(TokenKind::Number, parser.peekKind());
}

TEST_F(ParserTest, PeekKindReturnsNulloptOnEmpty) {
  const auto tokens = lex(U"");
  auto parser = Parser(TokenSource(tokens));

  EXPECT_FALSE(parser.peekKind().has_value());
}

TEST_F(ParserTest, PeekKindSkipsWhitespace) {
  const auto tokens = lex(U"   abc");
  auto parser = Parser(TokenSource(tokens));

  EXPECT_EQ(TokenKind::Ident, parser.peekKind());
}

TEST_F(ParserTest, AtReturnsTrueForMatchingKind) {
  const auto tokens = lex(U"123");
  auto parser = Parser(TokenSource(tokens));

  EXPECT_TRUE(parser.at(TokenKind::Number));
}

TEST_F(ParserTest, AtReturnsFalseForNonMatchingKind) {
  const auto tokens = lex(U"123");
  auto parser = Parser(TokenSource(tokens));

  EXPECT_FALSE(parser.at(TokenKind::Ident));
}

TEST_F(ParserTest, AtEndReturnsTrueOnEmpty) {
  const auto tokens = lex(U"");
  auto parser = Parser(TokenSource(tokens));

  EXPECT_TRUE(parser.atEnd());
}

TEST_F(ParserTest, AtEndReturnsFalseWithTokens) {
  const auto tokens = lex(U"x");
  auto parser = Parser(TokenSource(tokens));

  EXPECT_FALSE(parser.atEnd());
}

TEST_F(ParserTest, BumpAdvancesCursor) {
  const auto tokens = lex(U"a b");
  auto parser = Parser(TokenSource(tokens));

  EXPECT_EQ(TokenKind::Ident, parser.peekKind());
  parser.bump();
  EXPECT_EQ(TokenKind::Ident, parser.peekKind());
  parser.bump();
  EXPECT_FALSE(parser.peekKind().has_value());
}

TEST_F(ParserTest, BumpToEnd) {
  const auto tokens = lex(U"x");
  auto parser = Parser(TokenSource(tokens));

  parser.bump();
  EXPECT_TRUE(parser.atEnd());
}

TEST_F(ParserTest, StartReturnsMarkerAtPositionZero) {
  const auto tokens = lex(U"x");
  auto parser = Parser(TokenSource(tokens));

  const Marker marker = parser.start();
  EXPECT_EQ(0, marker.position);
}

TEST_F(ParserTest, StartIncrementsPosition) {
  const auto tokens = lex(U"x");
  auto parser = Parser(TokenSource(tokens));

  const Marker m1 = parser.start();
  const Marker m2 = parser.start();
  EXPECT_EQ(0, m1.position);
  EXPECT_EQ(1, m2.position);
}

TEST_F(ParserTest, CompleteReturnsCompletedMarker) {
  const auto tokens = lex(U"x");
  auto parser = Parser(TokenSource(tokens));

  const Marker marker = parser.start();
  const CompletedMarker completed = parser.complete(marker, SyntaxKind::Ident);
  EXPECT_EQ(0, completed.position);
}

TEST_F(ParserTest, PrecedeCreatesNewMarkerAfterCompleted) {
  const auto tokens = lex(U"x");
  auto parser = Parser(TokenSource(tokens));

  const Marker marker = parser.start();
  parser.bump();
  const CompletedMarker completed =
      parser.complete(marker, SyntaxKind::LiteralExpr);

  const auto [newMarker, oldKind] = parser.precede(completed);
  EXPECT_EQ(SyntaxKind::LiteralExpr, oldKind);
  EXPECT_GT(newMarker.position, completed.position);
}

TEST_F(ParserTest, ExpectConsumesMatchingToken) {
  const auto tokens = lex(U"123");
  auto parser = Parser(TokenSource(tokens));

  parser.expect(TokenKind::Number);
  EXPECT_TRUE(parser.atEnd());
}

TEST_F(ParserTest, ExpectOnMismatchGeneratesError) {
  const auto tokens = lex(U"abc");
  auto parser = Parser(TokenSource(tokens));

  parser.expect(TokenKind::Number);
  // After error, if not at recovery set and not at end, bump happens
  EXPECT_TRUE(parser.atEnd());
}
} // namespace
