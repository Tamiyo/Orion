#include "../ParserTestUtils.h"

#include <gtest/gtest.h>

namespace {
class QueryTest : public yuzu::parser::test::ParserFixture {};

// `from <rel> [as] <alias> |> select <item> [as <alias>]` nests as
// `SelectExpr(FromExpr(...))`. Queries parse at statement position, so the test
// drives `parseStmt` rather than `parseExpr`.
TEST_F(QueryTest, ParsesFromSelectQuery) {
  const auto result = parseStmt(U"from employees e |> select id as x");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"(ExprStmt@0..34
  SelectExpr@0..34
    FromExpr@0..17
      FromKw@0..4 "from"
      Space@4..5 " "
      Ident@5..15
        Identifier@5..14 "employees"
        Space@14..15 " "
      Ident@15..17
        Identifier@15..16 "e"
        Space@16..17 " "
    Pipe@17..19 "|>"
    Space@19..20 " "
    SelectKw@20..26 "select"
    Space@26..27 " "
    SelectItem@27..34
      IdentExpr@27..30
        Ident@27..30
          Identifier@27..29 "id"
          Space@29..30 " "
      AsKw@30..32 "as"
      Space@32..33 " "
      Ident@33..34
        Identifier@33..34 "x")",
            result.tree);
}

// A bare `from` with no pipe stage is a complete query on its own.
TEST_F(QueryTest, ParsesBareFromQuery) {
  const auto result = parseStmt(U"from employees e");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"(ExprStmt@0..16
  FromExpr@0..16
    FromKw@0..4 "from"
    Space@4..5 " "
    Ident@5..15
      Identifier@5..14 "employees"
      Space@14..15 " "
    Ident@15..16
      Identifier@15..16 "e")",
            result.tree);
}

TEST_F(QueryTest, ParsesStructDecl) {
  const auto result = parseStmt(U"struct Point { x: int32, y: int32 }");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"(StructStmt@0..35
  StructKw@0..6 "struct"
  Space@6..7 " "
  Ident@7..13
    Identifier@7..12 "Point"
    Space@12..13 " "
  LeftCurly@13..14 "{"
  Space@14..15 " "
  StructFieldDecl@15..23
    Ident@15..16
      Identifier@15..16 "x"
    Colon@16..17 ":"
    Space@17..18 " "
    NamedTypeAnnotation@18..23
      Ident@18..23
        Identifier@18..23 "int32"
  Comma@23..24 ","
  Space@24..25 " "
  StructFieldDecl@25..34
    Ident@25..26
      Identifier@25..26 "y"
    Colon@26..27 ":"
    Space@27..28 " "
    NamedTypeAnnotation@28..34
      Ident@28..34
        Identifier@28..33 "int32"
        Space@33..34 " "
  RightCurly@34..35 "}")",
            result.tree);
}

TEST_F(QueryTest, ParsesNamedTable) {
  const auto result = parseStmt(U"table points = Point");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"(TableStmt@0..20
  TableKw@0..5 "table"
  Space@5..6 " "
  Ident@6..13
    Identifier@6..12 "points"
    Space@12..13 " "
  Eq@13..14 "="
  Space@14..15 " "
  Ident@15..20
    Identifier@15..20 "Point")",
            result.tree);
}

TEST_F(QueryTest, ParsesInlineTable) {
  const auto result = parseStmt(U"table events = { id: int32 }");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"(TableStmt@0..28
  TableKw@0..5 "table"
  Space@5..6 " "
  Ident@6..13
    Identifier@6..12 "events"
    Space@12..13 " "
  Eq@13..14 "="
  Space@14..15 " "
  LeftCurly@15..16 "{"
  Space@16..17 " "
  StructFieldDecl@17..27
    Ident@17..19
      Identifier@17..19 "id"
    Colon@19..20 ":"
    Space@20..21 " "
    NamedTypeAnnotation@21..27
      Ident@21..27
        Identifier@21..26 "int32"
        Space@26..27 " "
  RightCurly@27..28 "}")",
            result.tree);
}

TEST_F(QueryTest, ParsesFieldAccess) {
  const auto result = parseExpr(U"e.id");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"(Expr@0..4
  FieldAccessExpr@0..4
    IdentExpr@0..1
      Ident@0..1
        Identifier@0..1 "e"
    Dot@1..2 "."
    Ident@2..4
      Identifier@2..4 "id")",
            result.tree);
}

TEST_F(QueryTest, ParsesStructLiteral) {
  const auto result = parseExpr(U"Employee { id: 1 }");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"(Expr@0..18
  StructLitExpr@0..18
    Ident@0..9
      Identifier@0..8 "Employee"
      Space@8..9 " "
    LeftCurly@9..10 "{"
    Space@10..11 " "
    StructLitField@11..17
      Ident@11..13
        Identifier@11..13 "id"
      Colon@13..14 ":"
      Space@14..15 " "
      IntLit@15..17
        IntegerLiteral@15..16 "1"
        Space@16..17 " "
    RightCurly@17..18 "}")",
            result.tree);
}
} // namespace
