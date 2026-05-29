#include "../ParserTestUtils.h"

#include <gtest/gtest.h>

namespace {
class ExprTest : public yuzu::parser::test::ParserFixture {};

TEST_F(ExprTest, ParsesNumberLiteral) {
  const auto result = parseExpr(U"42");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"(Expr@0..2
  IntLit@0..2
    IntegerLiteral@0..2 "42")",
            result.tree);
}

TEST_F(ExprTest, ParsesIdentifier) {
  const auto result = parseExpr(U"foo");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  // The schema declares `IdentExpr` as `Child<Ident>:$name`, so the
  // identifier is wrapped in a nested `Ident` node rather than
  // having the `Identifier` token directly under `IdentExpr`. That
  // way `lowerIdentExpr` can lower the child `Ident` the same way
  // as `LetStmt.name`.
  EXPECT_EQ(R"(Expr@0..3
  IdentExpr@0..3
    Ident@0..3
      Identifier@0..3 "foo")",
            result.tree);
}

TEST_F(ExprTest, ParsesAddition) {
  const auto result = parseExpr(U"1 + 2");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"(Expr@0..5
  BinaryExpr@0..5
    IntLit@0..2
      IntegerLiteral@0..1 "1"
      Space@1..2 " "
    Plus@2..3 "+"
    Space@3..4 " "
    IntLit@4..5
      IntegerLiteral@4..5 "2")",
            result.tree);
}

TEST_F(ExprTest, ParsesSubtraction) {
  const auto result = parseExpr(U"5 - 3");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"(Expr@0..5
  BinaryExpr@0..5
    IntLit@0..2
      IntegerLiteral@0..1 "5"
      Space@1..2 " "
    Minus@2..3 "-"
    Space@3..4 " "
    IntLit@4..5
      IntegerLiteral@4..5 "3")",
            result.tree);
}

TEST_F(ExprTest, ParsesMultiplication) {
  const auto result = parseExpr(U"2 * 3");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"(Expr@0..5
  BinaryExpr@0..5
    IntLit@0..2
      IntegerLiteral@0..1 "2"
      Space@1..2 " "
    Star@2..3 "*"
    Space@3..4 " "
    IntLit@4..5
      IntegerLiteral@4..5 "3")",
            result.tree);
}

TEST_F(ExprTest, ParsesDivision) {
  const auto result = parseExpr(U"8 / 2");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"(Expr@0..5
  BinaryExpr@0..5
    IntLit@0..2
      IntegerLiteral@0..1 "8"
      Space@1..2 " "
    Slash@2..3 "/"
    Space@3..4 " "
    IntLit@4..5
      IntegerLiteral@4..5 "2")",
            result.tree);
}

// `1 + 2 * 3` parses as `1 + (2 * 3)` — multiplication binds tighter than
// addition. The right-hand side of the outer `+` is a nested `BinaryExpr`.
TEST_F(ExprTest, MultiplicationBindsTighterThanAddition) {
  const auto result = parseExpr(U"1 + 2 * 3");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"(Expr@0..9
  BinaryExpr@0..9
    IntLit@0..2
      IntegerLiteral@0..1 "1"
      Space@1..2 " "
    Plus@2..3 "+"
    Space@3..4 " "
    BinaryExpr@4..9
      IntLit@4..6
        IntegerLiteral@4..5 "2"
        Space@5..6 " "
      Star@6..7 "*"
      Space@7..8 " "
      IntLit@8..9
        IntegerLiteral@8..9 "3")",
            result.tree);
}

// `1 + 2 + 3` parses left-associatively as `(1 + 2) + 3` — the left-hand
// side of the outer `+` is a nested `BinaryExpr`.
TEST_F(ExprTest, AdditionIsLeftAssociative) {
  const auto result = parseExpr(U"1 + 2 + 3");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"(Expr@0..9
  BinaryExpr@0..9
    BinaryExpr@0..6
      IntLit@0..2
        IntegerLiteral@0..1 "1"
        Space@1..2 " "
      Plus@2..3 "+"
      Space@3..4 " "
      IntLit@4..6
        IntegerLiteral@4..5 "2"
        Space@5..6 " "
    Plus@6..7 "+"
    Space@7..8 " "
    IntLit@8..9
      IntegerLiteral@8..9 "3")",
            result.tree);
}

TEST_F(ExprTest, ParsesLessThan) {
  const auto result = parseExpr(U"1 < 2");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"(Expr@0..5
  BinaryExpr@0..5
    IntLit@0..2
      IntegerLiteral@0..1 "1"
      Space@1..2 " "
    Lt@2..3 "<"
    Space@3..4 " "
    IntLit@4..5
      IntegerLiteral@4..5 "2")",
            result.tree);
}

TEST_F(ExprTest, ParsesLessThanOrEqual) {
  const auto result = parseExpr(U"1 <= 2");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"(Expr@0..6
  BinaryExpr@0..6
    IntLit@0..2
      IntegerLiteral@0..1 "1"
      Space@1..2 " "
    Lte@2..4 "<="
    Space@4..5 " "
    IntLit@5..6
      IntegerLiteral@5..6 "2")",
            result.tree);
}

TEST_F(ExprTest, ParsesGreaterThan) {
  const auto result = parseExpr(U"1 > 2");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"(Expr@0..5
  BinaryExpr@0..5
    IntLit@0..2
      IntegerLiteral@0..1 "1"
      Space@1..2 " "
    Gt@2..3 ">"
    Space@3..4 " "
    IntLit@4..5
      IntegerLiteral@4..5 "2")",
            result.tree);
}

TEST_F(ExprTest, ParsesGreaterThanOrEqual) {
  const auto result = parseExpr(U"1 >= 2");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"(Expr@0..6
  BinaryExpr@0..6
    IntLit@0..2
      IntegerLiteral@0..1 "1"
      Space@1..2 " "
    Gte@2..4 ">="
    Space@4..5 " "
    IntLit@5..6
      IntegerLiteral@5..6 "2")",
            result.tree);
}

TEST_F(ExprTest, ParsesShiftLeft) {
  const auto result = parseExpr(U"1 << 2");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"(Expr@0..6
  BinaryExpr@0..6
    IntLit@0..2
      IntegerLiteral@0..1 "1"
      Space@1..2 " "
    ShiftLeft@2..4 "<<"
    Space@4..5 " "
    IntLit@5..6
      IntegerLiteral@5..6 "2")",
            result.tree);
}

TEST_F(ExprTest, ParsesShiftRight) {
  const auto result = parseExpr(U"1 >> 2");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"(Expr@0..6
  BinaryExpr@0..6
    IntLit@0..2
      IntegerLiteral@0..1 "1"
      Space@1..2 " "
    ShiftRight@2..4 ">>"
    Space@4..5 " "
    IntLit@5..6
      IntegerLiteral@5..6 "2")",
            result.tree);
}

TEST_F(ExprTest, ParsesIn) {
  const auto result = parseExpr(U"1 in 2");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"(Expr@0..6
  BinaryExpr@0..6
    IntLit@0..2
      IntegerLiteral@0..1 "1"
      Space@1..2 " "
    InKw@2..4 "in"
    Space@4..5 " "
    IntLit@5..6
      IntegerLiteral@5..6 "2")",
            result.tree);
}

// `not in` is a single binary operator spanning two tokens; both the `not`
// and `in` tokens land as children of the BinaryExpr.
TEST_F(ExprTest, ParsesNotIn) {
  const auto result = parseExpr(U"1 not in 2");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"(Expr@0..10
  BinaryExpr@0..10
    IntLit@0..2
      IntegerLiteral@0..1 "1"
      Space@1..2 " "
    NotKw@2..5 "not"
    Space@5..6 " "
    InKw@6..8 "in"
    Space@8..9 " "
    IntLit@9..10
      IntegerLiteral@9..10 "2")",
            result.tree);
}

// Prefix `not` produces a UnaryExpr wrapping its operand.
TEST_F(ExprTest, ParsesNot) {
  const auto result = parseExpr(U"not 1");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"(Expr@0..5
  UnaryExpr@0..5
    NotKw@0..3 "not"
    Space@3..4 " "
    IntLit@4..5
      IntegerLiteral@4..5 "1")",
            result.tree);
}

// `1 < 2 << 3` parses as `1 < (2 << 3)` — shifts bind tighter than
// comparisons.
TEST_F(ExprTest, ShiftBindsTighterThanComparison) {
  const auto result = parseExpr(U"1 < 2 << 3");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"(Expr@0..10
  BinaryExpr@0..10
    IntLit@0..2
      IntegerLiteral@0..1 "1"
      Space@1..2 " "
    Lt@2..3 "<"
    Space@3..4 " "
    BinaryExpr@4..10
      IntLit@4..6
        IntegerLiteral@4..5 "2"
        Space@5..6 " "
      ShiftLeft@6..8 "<<"
      Space@8..9 " "
      IntLit@9..10
        IntegerLiteral@9..10 "3")",
            result.tree);
}

// `1 << 2 + 3` parses as `1 << (2 + 3)` — addition binds tighter than
// shifts.
TEST_F(ExprTest, AdditionBindsTighterThanShift) {
  const auto result = parseExpr(U"1 << 2 + 3");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"(Expr@0..10
  BinaryExpr@0..10
    IntLit@0..2
      IntegerLiteral@0..1 "1"
      Space@1..2 " "
    ShiftLeft@2..4 "<<"
    Space@4..5 " "
    BinaryExpr@5..10
      IntLit@5..7
        IntegerLiteral@5..6 "2"
        Space@6..7 " "
      Plus@7..8 "+"
      Space@8..9 " "
      IntLit@9..10
        IntegerLiteral@9..10 "3")",
            result.tree);
}

// `not 1 < 2` parses as `not (1 < 2)` — `not` binds looser than comparison,
// so its operand swallows the whole comparison.
TEST_F(ExprTest, NotBindsLooserThanComparison) {
  const auto result = parseExpr(U"not 1 < 2");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"(Expr@0..9
  UnaryExpr@0..9
    NotKw@0..3 "not"
    Space@3..4 " "
    BinaryExpr@4..9
      IntLit@4..6
        IntegerLiteral@4..5 "1"
        Space@5..6 " "
      Lt@6..7 "<"
      Space@7..8 " "
      IntLit@8..9
        IntegerLiteral@8..9 "2")",
            result.tree);
}

// `1 +` should still wrap the LHS+operator in a BinaryExpr and report one
// error for the missing RHS — partial trees are how the parser stays
// recoverable. The recursive `parseLhs` hits end-of-input, so the error
// reads "found end of input" and the error span falls back to the last
// consumed token (the `+`).
TEST_F(ExprTest, RecoversFromMissingRhs) {
  const auto result = parseExpr(U"1 +");

  ASSERT_EQ(1u, diagnostics.getDiagnostics().size());
  const auto &d = diagnostics.getDiagnostics()[0];
  EXPECT_EQ(yuzu::diagnostics::Severity::Error, d.severity);
  EXPECT_EQ("expected expression, found end of input", d.message);
  ASSERT_EQ(1u, d.labels.size());
  EXPECT_EQ(2u, d.labels[0].span.start);
  EXPECT_EQ(3u, d.labels[0].span.end);

  EXPECT_EQ(R"(Expr@0..3
  BinaryExpr@0..3
    IntLit@0..2
      IntegerLiteral@0..1 "1"
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

  ASSERT_EQ(1u, diagnostics.getDiagnostics().size());
  const auto &d = diagnostics.getDiagnostics()[0];
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
