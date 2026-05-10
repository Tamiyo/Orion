#include "../ParserTestUtils.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace {
using yuzu::parser::test::parseStmt;

// `parseStmt` currently delegates straight to `parseExpr`, so the tree
// under the test harness's `Stmt` wrapper looks identical to the
// expression grammar's output — only the outer kind changes.
TEST(StmtTest, ParsesNumberLiteral) {
  const auto result = parseStmt(U"42");

  EXPECT_EQ(std::vector<std::string>{}, result.errors);
  EXPECT_EQ(R"(Stmt@0..2
  LiteralExpr@0..2
    Number@0..2 "42")",
            result.tree);
}

TEST(StmtTest, ParsesBinaryExpression) {
  const auto result = parseStmt(U"1 + 2");

  EXPECT_EQ(std::vector<std::string>{}, result.errors);
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
TEST(StmtTest, ReportsErrorOnMissingLhs) {
  const auto result = parseStmt(U"+ 1");

  EXPECT_EQ((std::vector<std::string>{
                "parser error at 0, 1 - found Plus but expected one of []"}),
            result.errors);
  EXPECT_EQ(R"(Stmt@0..2
  Error@0..2
    Plus@0..1 "+"
    Space@1..2 " ")",
            result.tree);
}

} // namespace
