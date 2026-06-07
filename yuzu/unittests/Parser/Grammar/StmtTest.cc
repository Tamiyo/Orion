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
  NamedTypeAnnotation@6..9
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

//===----------------------------------------------------------------------===//
// Functions
//===----------------------------------------------------------------------===//

// Minimal function: no params, no return type, a block with one return.
TEST_F(StmtTest, ParsesFnNoParams) {
  const auto result = parseStmt(U"fn f(){return 1}");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"tree(FuncStmt@0..16
  FnKw@0..2 "fn"
  Space@2..3 " "
  Ident@3..4
    Identifier@3..4 "f"
  LeftParen@4..5 "("
  RightParen@5..6 ")"
  BlockStmt@6..16
    LeftCurly@6..7 "{"
    ReturnStmt@7..15
      ReturnKw@7..13 "return"
      Space@13..14 " "
      IntLit@14..15
        IntegerLiteral@14..15 "1"
    RightCurly@15..16 "}")tree",
            result.tree);
}

// Comma-separated params and a return type annotation.
TEST_F(StmtTest, ParsesFnParamsAndReturnType) {
  const auto result = parseStmt(U"fn add(x:int32,y:int32)->int32{return x+y}");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"tree(FuncStmt@0..42
  FnKw@0..2 "fn"
  Space@2..3 " "
  Ident@3..6
    Identifier@3..6 "add"
  LeftParen@6..7 "("
  Param@7..14
    Ident@7..8
      Identifier@7..8 "x"
    Colon@8..9 ":"
    NamedTypeAnnotation@9..14
      Ident@9..14
        Identifier@9..14 "int32"
  Comma@14..15 ","
  Param@15..22
    Ident@15..16
      Identifier@15..16 "y"
    Colon@16..17 ":"
    NamedTypeAnnotation@17..22
      Ident@17..22
        Identifier@17..22 "int32"
  RightParen@22..23 ")"
  Arrow@23..25 "->"
  NamedTypeAnnotation@25..30
    Ident@25..30
      Identifier@25..30 "int32"
  BlockStmt@30..42
    LeftCurly@30..31 "{"
    ReturnStmt@31..41
      ReturnKw@31..37 "return"
      Space@37..38 " "
      BinaryExpr@38..41
        IdentExpr@38..39
          Ident@38..39
            Identifier@38..39 "x"
        Plus@39..40 "+"
        IdentExpr@40..41
          Ident@40..41
            Identifier@40..41 "y"
    RightCurly@41..42 "}")tree",
            result.tree);
}

// A single generic type parameter `[T]`, used in a param and the return.
TEST_F(StmtTest, ParsesFnSingleTypeParam) {
  const auto result = parseStmt(U"fn id[T](x:T)->T{return x}");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"tree(FuncStmt@0..26
  FnKw@0..2 "fn"
  Space@2..3 " "
  Ident@3..5
    Identifier@3..5 "id"
  LeftBracket@5..6 "["
  TypeParam@6..7
    Ident@6..7
      Identifier@6..7 "T"
  RightBracket@7..8 "]"
  LeftParen@8..9 "("
  Param@9..12
    Ident@9..10
      Identifier@9..10 "x"
    Colon@10..11 ":"
    NamedTypeAnnotation@11..12
      Ident@11..12
        Identifier@11..12 "T"
  RightParen@12..13 ")"
  Arrow@13..15 "->"
  NamedTypeAnnotation@15..16
    Ident@15..16
      Identifier@15..16 "T"
  BlockStmt@16..26
    LeftCurly@16..17 "{"
    ReturnStmt@17..25
      ReturnKw@17..23 "return"
      Space@23..24 " "
      IdentExpr@24..25
        Ident@24..25
          Identifier@24..25 "x"
    RightCurly@25..26 "}")tree",
            result.tree);
}

// Multiple comma-separated type parameters `[T, U]`.
TEST_F(StmtTest, ParsesFnMultipleTypeParams) {
  const auto result = parseStmt(U"fn two[T,U](a:T,b:U){return a}");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"tree(FuncStmt@0..30
  FnKw@0..2 "fn"
  Space@2..3 " "
  Ident@3..6
    Identifier@3..6 "two"
  LeftBracket@6..7 "["
  TypeParam@7..8
    Ident@7..8
      Identifier@7..8 "T"
  Comma@8..9 ","
  TypeParam@9..10
    Ident@9..10
      Identifier@9..10 "U"
  RightBracket@10..11 "]"
  LeftParen@11..12 "("
  Param@12..15
    Ident@12..13
      Identifier@12..13 "a"
    Colon@13..14 ":"
    NamedTypeAnnotation@14..15
      Ident@14..15
        Identifier@14..15 "T"
  Comma@15..16 ","
  Param@16..19
    Ident@16..17
      Identifier@16..17 "b"
    Colon@17..18 ":"
    NamedTypeAnnotation@18..19
      Ident@18..19
        Identifier@18..19 "U"
  RightParen@19..20 ")"
  BlockStmt@20..30
    LeftCurly@20..21 "{"
    ReturnStmt@21..29
      ReturnKw@21..27 "return"
      Space@27..28 " "
      IdentExpr@28..29
        Ident@28..29
          Identifier@28..29 "a"
    RightCurly@29..30 "}")tree",
            result.tree);
}

//===----------------------------------------------------------------------===//
// Return statements
//===----------------------------------------------------------------------===//

// `return expr` keeps the operand as a child.
TEST_F(StmtTest, ParsesReturnWithValue) {
  const auto result = parseStmt(U"return 5");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"(ReturnStmt@0..8
  ReturnKw@0..6 "return"
  Space@6..7 " "
  IntLit@7..8
    IntegerLiteral@7..8 "5")",
            result.tree);
}

} // namespace
