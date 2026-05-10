#include "../ParserTestUtils.h"

#include <gtest/gtest.h>

#include <vector>

namespace {
using yuzu::parser::test::parseExpr;

TEST(ExprTest, ParsesNumberLiteral) {
  const auto result = parseExpr(U"42");

  EXPECT_TRUE(result.errors.empty());
  EXPECT_EQ(R"(Expr@0..2
  LiteralExpr@0..2
    Number@0..2 "42")",
            result.tree);
}

TEST(ExprTest, ParsesIdentifier) {
  const auto result = parseExpr(U"foo");

  EXPECT_TRUE(result.errors.empty());
  // The parser tags an identifier node with the same `Ident` kind as the
  // underlying token, hence the doubled `Ident@..`.
  EXPECT_EQ(R"(Expr@0..3
  Ident@0..3
    Ident@0..3 "foo")",
            result.tree);
}

TEST(ExprTest, ParsesAddition) {
  const auto result = parseExpr(U"1 + 2");

  EXPECT_TRUE(result.errors.empty());
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

TEST(ExprTest, ParsesSubtraction) {
  const auto result = parseExpr(U"5 - 3");

  EXPECT_TRUE(result.errors.empty());
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

TEST(ExprTest, ParsesMultiplication) {
  const auto result = parseExpr(U"2 * 3");

  EXPECT_TRUE(result.errors.empty());
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

TEST(ExprTest, ParsesDivision) {
  const auto result = parseExpr(U"8 / 2");

  EXPECT_TRUE(result.errors.empty());
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
TEST(ExprTest, MultiplicationBindsTighterThanAddition) {
  const auto result = parseExpr(U"1 + 2 * 3");

  EXPECT_TRUE(result.errors.empty());
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
TEST(ExprTest, AdditionIsLeftAssociative) {
  const auto result = parseExpr(U"1 + 2 + 3");

  EXPECT_TRUE(result.errors.empty());
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
// recoverable. The recursive `parseLhs` hits end-of-input, so `found` is
// reported as `None` and the error range falls back to the last consumed
// token (the `+`).
TEST(ExprTest, RecoversFromMissingRhs) {
  const auto result = parseExpr(U"1 +");

  EXPECT_EQ((std::vector<std::string>{
                "parser error at 2, 3 - found None but expected one of [Number, Ident, LeftParen]"}),
            result.errors);
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
// attaches as trivia). `expectedKinds` lists every kind `parseLhs` would
// have accepted, since each branch goes through `p.at(...)`.
TEST(ExprTest, RecoversFromMissingLhs) {
  const auto result = parseExpr(U"+ 1");

  EXPECT_EQ((std::vector<std::string>{
                "parser error at 0, 1 - found Plus but expected one of [Number, Ident, LeftParen]"}),
            result.errors);
  EXPECT_EQ(R"(Expr@0..2
  Error@0..2
    Plus@0..1 "+"
    Space@1..2 " ")",
            result.tree);
}

} // namespace
