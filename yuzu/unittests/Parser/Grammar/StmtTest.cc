#include "../ParserTestUtils.h"

#include <gtest/gtest.h>

namespace {
class StmtTest : public yuzu::parser::test::ParserFixture {};

TEST_F(StmtTest, ParsesNumberLiteral) {
  const auto result = parseStmt(U"42");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"(ExprStmt@0..2
  IntLit@0..2
    IntegerLiteral@0..2 "42")",
            result.tree);
}

TEST_F(StmtTest, ParsesBinaryExpression) {
  const auto result = parseStmt(U"1 + 2");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"(ExprStmt@0..5
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

// A `let` with no annotation: the optional `type` child is simply absent.
TEST_F(StmtTest, ParsesLetWithoutAnnotation) {
  const auto result = parseStmt(U"let x=5");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"(LetStmt@0..7
  LetKw@0..3 "let"
  Space@3..4 " "
  Ident@4..5
    Identifier@4..5 "x"
  Eq@5..6 "="
  IntLit@6..7
    IntegerLiteral@6..7 "5")",
            result.tree);
}

// A `let` with an annotation: `: int` parses into a `NamedType` child
// between the name and the `=`.
TEST_F(StmtTest, ParsesLetWithAnnotation) {
  const auto result = parseStmt(U"let x:int=5");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"(LetStmt@0..11
  LetKw@0..3 "let"
  Space@3..4 " "
  Ident@4..5
    Identifier@4..5 "x"
  Colon@5..6 ":"
  NamedType@6..9
    Ident@6..9
      Identifier@6..9 "int"
  Eq@9..10 "="
  IntLit@10..11
    IntegerLiteral@10..11 "5")",
            result.tree);
}

// `let x:=5` — the annotation colon must be followed by a type, so the
// `=` where a type was expected is reported.
TEST_F(StmtTest, ReportsErrorOnMissingAnnotationType) {
  (void)parseStmt(U"let x:=5");

  ASSERT_FALSE(diagnostics.getDiagnostics().empty());
  EXPECT_NE(diagnostics.getDiagnostics()[0].message.find("expected"),
            std::string::npos)
      << "first diagnostic: " << diagnostics.getDiagnostics()[0].message;
}

// `*` is used rather than `+`/`-` because those are prefix operators and
// would parse as a UnaryExpr instead of triggering missing-LHS recovery.
TEST_F(StmtTest, ReportsErrorOnMissingLhs) {
  const auto result = parseStmt(U"* 1");

  ASSERT_EQ(1u, diagnostics.getDiagnostics().size());
  const auto &d = diagnostics.getDiagnostics()[0];
  EXPECT_EQ(yuzu::diagnostics::Severity::Error, d.severity);
  EXPECT_EQ("expected expression, found `*`", d.message);
  ASSERT_EQ(1u, d.labels.size());
  EXPECT_EQ(0u, d.labels[0].span.start);
  EXPECT_EQ(1u, d.labels[0].span.end);

  EXPECT_EQ(R"(ExprStmt@0..2
  Error@0..2
    Star@0..1 "*"
    Space@1..2 " ")",
            result.tree);
}

} // namespace
