#include "../ParserTestUtils.h"

#include <gtest/gtest.h>

namespace {
class TypeTest : public yuzu::parser::test::ParserFixture {};

// A bare name is a `NamedType` with no arguments.
TEST_F(TypeTest, ParsesBareName) {
  const auto result = parseType(U"string");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"tree(NamedTypeAnnotation@0..6
  Ident@0..6
    Identifier@0..6 "string")tree",
            result.tree);
}

// `Relation[Employee]` — a name applied to one type argument, itself a
// nested `NamedType`.
TEST_F(TypeTest, ParsesSingleTypeArgument) {
  const auto result = parseType(U"Relation[Employee]");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"tree(NamedTypeAnnotation@0..18
  Ident@0..8
    Identifier@0..8 "Relation"
  LeftBracket@8..9 "["
  NamedTypeAnnotation@9..17
    Ident@9..17
      Identifier@9..17 "Employee"
  RightBracket@17..18 "]")tree",
            result.tree);
}

// Multiple comma-separated arguments: `Aggregate[decimal,int64]`.
TEST_F(TypeTest, ParsesMultipleTypeArguments) {
  const auto result = parseType(U"Aggregate[decimal,int64]");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"tree(NamedTypeAnnotation@0..24
  Ident@0..9
    Identifier@0..9 "Aggregate"
  LeftBracket@9..10 "["
  NamedTypeAnnotation@10..17
    Ident@10..17
      Identifier@10..17 "decimal"
  Comma@17..18 ","
  NamedTypeAnnotation@18..23
    Ident@18..23
      Identifier@18..23 "int64"
  RightBracket@23..24 "]")tree",
            result.tree);
}

// Nesting falls out of the recursion: `List[Relation[Employee]]`.
TEST_F(TypeTest, ParsesNestedGenerics) {
  const auto result = parseType(U"List[Relation[Employee]]");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"tree(NamedTypeAnnotation@0..24
  Ident@0..4
    Identifier@0..4 "List"
  LeftBracket@4..5 "["
  NamedTypeAnnotation@5..23
    Ident@5..13
      Identifier@5..13 "Relation"
    LeftBracket@13..14 "["
    NamedTypeAnnotation@14..22
      Ident@14..22
        Identifier@14..22 "Employee"
    RightBracket@22..23 "]"
  RightBracket@23..24 "]")tree",
            result.tree);
}

// `(int,str)->bool` — a function type with a positional parameter list and
// a result after the arrow.
TEST_F(TypeTest, ParsesFunctionType) {
  const auto result = parseType(U"(int,str)->bool");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"tree(FuncTypeAnnotation@0..15
  FuncTypeAnnotationParams@0..9
    LeftParen@0..1 "("
    NamedTypeAnnotation@1..4
      Ident@1..4
        Identifier@1..4 "int"
    Comma@4..5 ","
    NamedTypeAnnotation@5..8
      Ident@5..8
        Identifier@5..8 "str"
    RightParen@8..9 ")"
  Arrow@9..11 "->"
  NamedTypeAnnotation@11..15
    Ident@11..15
      Identifier@11..15 "bool")tree",
            result.tree);
}

// A function with no parameters still needs the arrow: `()->bool`.
TEST_F(TypeTest, ParsesNullaryFunctionType) {
  const auto result = parseType(U"()->bool");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"tree(FuncTypeAnnotation@0..8
  FuncTypeAnnotationParams@0..2
    LeftParen@0..1 "("
    RightParen@1..2 ")"
  Arrow@2..4 "->"
  NamedTypeAnnotation@4..8
    Ident@4..8
      Identifier@4..8 "bool")tree",
            result.tree);
}

// `(sum:decimal,count:int64)` — an anonymous record. The `Identifier ':'`
// lookahead steers `(` to a record rather than a function type.
TEST_F(TypeTest, ParsesRecordType) {
  const auto result = parseType(U"(sum:decimal,count:int64)");

  EXPECT_TRUE(diagnostics.getDiagnostics().empty());
  EXPECT_EQ(R"tree(RecordType@0..25
  LeftParen@0..1 "("
  RecordField@1..12
    Ident@1..4
      Identifier@1..4 "sum"
    Colon@4..5 ":"
    NamedTypeAnnotation@5..12
      Ident@5..12
        Identifier@5..12 "decimal"
  Comma@12..13 ","
  RecordField@13..24
    Ident@13..18
      Identifier@13..18 "count"
    Colon@18..19 ":"
    NamedTypeAnnotation@19..24
      Ident@19..24
        Identifier@19..24 "int64"
  RightParen@24..25 ")")tree",
            result.tree);
}

//===----------------------------------------------------------------------===//
// Error recovery — malformed types must emit a diagnostic (and not hang or
// crash); `expect` drives the "expected X, found Y" message.
//===----------------------------------------------------------------------===//

// Each malformed type must surface at least one `expected …` diagnostic.
// Exact counts/messages are left loose because the parser's recovery
// accumulates expected kinds and keeps going, which is allowed to evolve.
void expectParseError(yuzu::diagnostics::DiagnosticsEngine &diagnostics) {
  ASSERT_FALSE(diagnostics.getDiagnostics().empty());
  EXPECT_NE(diagnostics.getDiagnostics()[0].message.find("expected"),
            std::string::npos)
      << "first diagnostic: " << diagnostics.getDiagnostics()[0].message;
}

// `Relation[Employee` — the closing bracket is missing.
TEST_F(TypeTest, UnclosedBracketEmitsError) {
  (void)parseType(U"Relation[Employee");
  expectParseError(diagnostics);
}

// `Relation[]` — a type argument is required between the brackets.
TEST_F(TypeTest, EmptyBracketsEmitError) {
  (void)parseType(U"Relation[]");
  expectParseError(diagnostics);
}

// `(int)bool` — a function type needs the `->` arrow before its result.
TEST_F(TypeTest, FunctionTypeMissingArrowEmitsError) {
  (void)parseType(U"(int)bool");
  expectParseError(diagnostics);
}

// `(int)->` — the result type after the arrow is missing.
TEST_F(TypeTest, FunctionTypeMissingResultEmitsError) {
  (void)parseType(U"(int)->");
  expectParseError(diagnostics);
}

} // namespace
