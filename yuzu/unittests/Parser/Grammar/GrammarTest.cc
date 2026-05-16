#include "../ParserTestUtils.h"

#include <gtest/gtest.h>

namespace {
class GrammarTest : public yuzu::parser::test::ParserFixture {};

// Empty input: `parseRoot` opens its `Root` marker, sees `atEnd`, closes
// without consuming anything. The resulting tree is just an empty Root
// spanning offsets 0..0.
TEST_F(GrammarTest, ParsesEmptyInput) {
  const auto result = parseRoot(U"");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ("Root@0..0", result.tree);
}

TEST_F(GrammarTest, ParsesSingleNumberLiteral) {
  const auto result = parseRoot(U"42");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"(Root@0..2
  ExprStmt@0..2
    IntLit@0..2
      IntegerLiteral@0..2 "42")",
            result.tree);
}

TEST_F(GrammarTest, ParsesSingleBinaryExpression) {
  const auto result = parseRoot(U"1 + 2");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"(Root@0..5
  ExprStmt@0..5
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

// Malformed input still yields a tree with a `Root` wrapper. The leading
// `+` triggers a missing-LHS error and is wrapped in an `Error` node
// inside its `ExprStmt`; `parseRoot` then recovers and parses the
// trailing `1` as a separate top-level statement.
TEST_F(GrammarTest, ReportsErrorOnMissingLhs) {
  const auto result = parseRoot(U"+ 1");

  ASSERT_EQ(1u, diagnostics.getDiagnostics().size());
  const auto &d = diagnostics.getDiagnostics()[0];
  EXPECT_EQ(yuzu::diagnostics::Severity::Error, d.severity);
  EXPECT_EQ("expected expression, found `+`", d.message);
  ASSERT_EQ(1u, d.labels.size());
  EXPECT_EQ(0u, d.labels[0].span.start);
  EXPECT_EQ(1u, d.labels[0].span.end);

  EXPECT_EQ(R"(Root@0..3
  ExprStmt@0..2
    Error@0..2
      Plus@0..1 "+"
      Space@1..2 " "
  ExprStmt@2..3
    IntLit@2..3
      IntegerLiteral@2..3 "1")",
            result.tree);
}

} // namespace
