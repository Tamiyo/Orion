#include "../ParserTestUtils.h"

#include <gtest/gtest.h>

namespace {
class StmtTest : public yuzu::parser::test::ParserFixture {};

// `parseStmt` currently delegates straight to `parseExpr`, so the tree
// under the test harness's `Stmt` wrapper looks identical to the
// expression grammar's output — only the outer kind changes.
TEST_F(StmtTest, ParsesNumberLiteral) {
  const auto result = parseStmt(U"42");

  EXPECT_TRUE(engine.getDiagnostics().empty());
  EXPECT_EQ(R"(Stmt@0..2
  LiteralExpr@0..2
    Number@0..2 "42")",
            result.tree);
}

TEST_F(StmtTest, ParsesBinaryExpression) {
  const auto result = parseStmt(U"1 + 2");

  EXPECT_TRUE(engine.getDiagnostics().empty());
  EXPECT_EQ(R"(Stmt@0..5
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

// `parseStmt` falls through to `parseExpr`, so an unexpected leading token
// surfaces the same `parseLhs` error and an `Error` node — wrapped in the
// `Stmt` marker rather than `Expr`.
TEST_F(StmtTest, ReportsErrorOnMissingLhs) {
  const auto result = parseStmt(U"+ 1");

  ASSERT_EQ(1u, engine.getDiagnostics().size());
  const auto &d = engine.getDiagnostics()[0];
  EXPECT_EQ(yuzu::diagnostics::Severity::Error, d.severity);
  EXPECT_EQ("found Plus but expected one of [Number, Ident, LeftParen]",
            d.message);
  ASSERT_EQ(1u, d.labels.size());
  EXPECT_EQ(0u, d.labels[0].span.start);
  EXPECT_EQ(1u, d.labels[0].span.end);

  EXPECT_EQ(R"(Stmt@0..2
  Error@0..2
    Plus@0..1 "+"
    Space@1..2 " ")",
            result.tree);
}

} // namespace
