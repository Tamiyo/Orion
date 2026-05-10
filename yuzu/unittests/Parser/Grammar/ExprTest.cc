#include "../ParserTestUtils.h"

#include <gtest/gtest.h>

namespace {
class ExprTest : public yuzu::parser::test::ParserFixture {};

TEST_F(ExprTest, ParsesNumberLiteral) {
  const auto result = parseExpr(U"42");

  EXPECT_TRUE(engine.getDiagnostics().empty());
  EXPECT_EQ(R"(Expr@0..2
  LiteralExpr@0..2
    Number@0..2 "42")",
            result.tree);
}

TEST_F(ExprTest, ParsesIdentifier) {
  const auto result = parseExpr(U"foo");

  EXPECT_TRUE(engine.getDiagnostics().empty());
  // The parser tags an identifier node with the same `Ident` kind as the
  // underlying token, hence the doubled `Ident@..`.
  EXPECT_EQ(R"(Expr@0..3
  Ident@0..3
    Ident@0..3 "foo")",
            result.tree);
}

TEST_F(ExprTest, ParsesAddition) {
  const auto result = parseExpr(U"1 + 2");

  EXPECT_TRUE(engine.getDiagnostics().empty());
  EXPECT_EQ(R"(Expr@0..5
  BinaryExpr@0..5
    LiteralExpr@0..2
      Number@0..1 "1"
      Space@1..2 " "
    Plus@2..3 "+"
    Space@3..4 " "
    LiteralExpr@4..5
      Number@4..5 "2")",
            result.tree);
}

TEST_F(ExprTest, ParsesSubtraction) {
  const auto result = parseExpr(U"5 - 3");

  EXPECT_TRUE(engine.getDiagnostics().empty());
  EXPECT_EQ(R"(Expr@0..5
  BinaryExpr@0..5
    LiteralExpr@0..2
      Number@0..1 "5"
      Space@1..2 " "
    Minus@2..3 "-"
    Space@3..4 " "
    LiteralExpr@4..5
      Number@4..5 "3")",
            result.tree);
}

TEST_F(ExprTest, ParsesMultiplication) {
  const auto result = parseExpr(U"2 * 3");

  EXPECT_TRUE(engine.getDiagnostics().empty());
  EXPECT_EQ(R"(Expr@0..5
  BinaryExpr@0..5
    LiteralExpr@0..2
      Number@0..1 "2"
      Space@1..2 " "
    Star@2..3 "*"
    Space@3..4 " "
    LiteralExpr@4..5
      Number@4..5 "3")",
            result.tree);
}

TEST_F(ExprTest, ParsesDivision) {
  const auto result = parseExpr(U"8 / 2");

  EXPECT_TRUE(engine.getDiagnostics().empty());
  EXPECT_EQ(R"(Expr@0..5
  BinaryExpr@0..5
    LiteralExpr@0..2
      Number@0..1 "8"
      Space@1..2 " "
    Slash@2..3 "/"
    Space@3..4 " "
    LiteralExpr@4..5
      Number@4..5 "2")",
            result.tree);
}

// `1 + 2 * 3` parses as `1 + (2 * 3)` — multiplication binds tighter than
// addition. The right-hand side of the outer `+` is a nested `BinaryExpr`.
TEST_F(ExprTest, MultiplicationBindsTighterThanAddition) {
  const auto result = parseExpr(U"1 + 2 * 3");

  EXPECT_TRUE(engine.getDiagnostics().empty());
  EXPECT_EQ(R"(Expr@0..9
  BinaryExpr@0..9
    LiteralExpr@0..2
      Number@0..1 "1"
      Space@1..2 " "
    Plus@2..3 "+"
    Space@3..4 " "
    BinaryExpr@4..9
      LiteralExpr@4..6
        Number@4..5 "2"
        Space@5..6 " "
      Star@6..7 "*"
      Space@7..8 " "
      LiteralExpr@8..9
        Number@8..9 "3")",
            result.tree);
}

// `1 + 2 + 3` parses left-associatively as `(1 + 2) + 3` — the left-hand
// side of the outer `+` is a nested `BinaryExpr`.
TEST_F(ExprTest, AdditionIsLeftAssociative) {
  const auto result = parseExpr(U"1 + 2 + 3");

  EXPECT_TRUE(engine.getDiagnostics().empty());
  EXPECT_EQ(R"(Expr@0..9
  BinaryExpr@0..9
    BinaryExpr@0..6
      LiteralExpr@0..2
        Number@0..1 "1"
        Space@1..2 " "
      Plus@2..3 "+"
      Space@3..4 " "
      LiteralExpr@4..6
        Number@4..5 "2"
        Space@5..6 " "
    Plus@6..7 "+"
    Space@7..8 " "
    LiteralExpr@8..9
      Number@8..9 "3")",
            result.tree);
}

// `1 +` should still wrap the LHS+operator in a BinaryExpr and report one
// error for the missing RHS — partial trees are how the parser stays
// recoverable. The recursive `parseLhs` hits end-of-input, so the error
// reads "found end of input" and the error span falls back to the last
// consumed token (the `+`).
TEST_F(ExprTest, RecoversFromMissingRhs) {
  const auto result = parseExpr(U"1 +");

  ASSERT_EQ(1u, engine.getDiagnostics().size());
  const auto &d = engine.getDiagnostics()[0];
  EXPECT_EQ(yuzu::diagnostics::Severity::Error, d.severity);
  EXPECT_EQ("expected expression, found end of input", d.message);
  ASSERT_EQ(1u, d.labels.size());
  EXPECT_EQ(2u, d.labels[0].span.start);
  EXPECT_EQ(3u, d.labels[0].span.end);

  EXPECT_EQ(R"(Expr@0..3
  BinaryExpr@0..3
    LiteralExpr@0..2
      Number@0..1 "1"
      Space@1..2 " "
    Plus@2..3 "+")",
            result.tree);
}

// `+ 1` has no LHS, so `parseLhs` reports an error against the unexpected
// `+` token and injects an `Error` node containing it (the trailing space
// attaches as trivia). The diagnostic quotes the actual source text in
// backticks rather than the lexer's kind name.
TEST_F(ExprTest, RecoversFromMissingLhs) {
  const auto result = parseExpr(U"+ 1");

  ASSERT_EQ(1u, engine.getDiagnostics().size());
  const auto &d = engine.getDiagnostics()[0];
  EXPECT_EQ(yuzu::diagnostics::Severity::Error, d.severity);
  EXPECT_EQ("expected expression, found `+`", d.message);
  ASSERT_EQ(1u, d.labels.size());
  EXPECT_EQ(0u, d.labels[0].span.start);
  EXPECT_EQ(1u, d.labels[0].span.end);

  EXPECT_EQ(R"(Expr@0..2
  Error@0..2
    Plus@0..1 "+"
    Space@1..2 " ")",
            result.tree);
}

} // namespace
