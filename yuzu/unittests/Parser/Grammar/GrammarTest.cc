#include "../ParserTestUtils.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace {
using yuzu::parser::test::parseRoot;

// Empty input: `parseRoot` opens its `Root` marker, sees `atEnd`, closes
// without consuming anything. The resulting tree is just an empty Root
// spanning offsets 0..0.
TEST(GrammarTest, ParsesEmptyInput) {
  const auto result = parseRoot(U"");

  EXPECT_EQ(std::vector<std::string>{}, result.errors);
  EXPECT_EQ("Root@0..0", result.tree);
}

// `parseStmt` currently forwards to `parseExpr`, which produces an
// `Expr`-kinded subtree directly under `Root` (there's no synthetic
// `Stmt` wrapper — see `Stmt.cc`).
TEST(GrammarTest, ParsesSingleNumberLiteral) {
  const auto result = parseRoot(U"42");

  EXPECT_EQ(std::vector<std::string>{}, result.errors);
  EXPECT_EQ(R"(Root@0..2
  LiteralExpr@0..2
    Number@0..2 "42")",
            result.tree);
}

TEST(GrammarTest, ParsesSingleBinaryExpression) {
  const auto result = parseRoot(U"1 + 2");

  EXPECT_EQ(std::vector<std::string>{}, result.errors);
  EXPECT_EQ(R"(Root@0..5
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

// Malformed input still yields a tree with a `Root` wrapper — the inner
// failure shows up as an `Error` node, mirroring how `parseExpr` surfaces
// missing-LHS errors.
TEST(GrammarTest, ReportsErrorOnMissingLhs) {
  const auto result = parseRoot(U"+ 1");

  EXPECT_EQ((std::vector<std::string>{
                "parser error at 0, 1 - found Plus but expected one of []"}),
            result.errors);
  EXPECT_EQ(R"(Root@0..2
  Error@0..2
    Plus@0..1 "+"
    Space@1..2 " ")",
            result.tree);
}

} // namespace
