#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

#include "lang/lexer/lexer.h"
#include "lang/lexer/token.h"
#include "lang/lexer/token_kind.h"
#include "lang/parser/grammar/expression/expression.h"
#include "lang/parser/parser.h"
#include "lang/parser/syntax_kind.h"
#include "lang/parser/token_sink.h"
#include "lang/parser/token_source.h"
#include "syntax/parser/rgtree/green/green_writer.h"

namespace {
using yuzu::lang::Expr;
using yuzu::lang::IsTrivia;
using yuzu::lang::Lexer;
using yuzu::lang::Parser;
using yuzu::lang::SyntaxKind;
using yuzu::lang::Token;
using yuzu::lang::TokenKind;
using yuzu::lang::TokenSink;
using yuzu::lang::TokenSource;
using yuzu::lang::ToU32String;

using yuzu::syntax::GreenWriter;

struct ExpressionTestCase {
  std::u32string source;
  std::u32string expected;
  std::string test_name;
};

class ExpressionParameterizedTestFixture
    : public testing::TestWithParam<ExpressionTestCase> {};

INSTANTIATE_TEST_SUITE_P(
    ParserTest, ExpressionParameterizedTestFixture,
    ::testing::Values(ExpressionTestCase{U"1+2",
                                         U"InfixExpr@0..3\n"
                                         U"  Literal@0..1\n"
                                         U"    IntLiteral@0..1 \"1\"\n"
                                         U"  Plus@1..2 \"+\"\n"
                                         U"  Literal@2..3\n"
                                         U"    IntLiteral@2..3 \"2\"\n",
                                         "Plus"},
                      ExpressionTestCase{U"1+2+3",
                                         U"InfixExpr@0..5\n"
                                         U"  InfixExpr@0..3\n"
                                         U"    Literal@0..1\n"
                                         U"      IntLiteral@0..1 \"1\"\n"
                                         U"    Plus@1..2 \"+\"\n"
                                         U"    Literal@2..3\n"
                                         U"      IntLiteral@2..3 \"2\"\n"
                                         U"  Plus@3..4 \"+\"\n"
                                         U"  Literal@4..5\n"
                                         U"    IntLiteral@4..5 \"3\"\n",
                                         "PlusMany"},
                      ExpressionTestCase{U"(1+2)+3",
                                         U"InfixExpr@0..7\n"
                                         U"  ParenExpr@0..5\n"
                                         U"    LeftParen@0..1 \"(\"\n"
                                         U"    InfixExpr@1..4\n"
                                         U"      Literal@1..2\n"
                                         U"        IntLiteral@1..2 \"1\"\n"
                                         U"      Plus@2..3 \"+\"\n"
                                         U"      Literal@3..4\n"
                                         U"        IntLiteral@3..4 \"2\"\n"
                                         U"    RightParen@4..5 \")\"\n"
                                         U"  Plus@5..6 \"+\"\n"
                                         U"  Literal@6..7\n"
                                         U"    IntLiteral@6..7 \"3\"\n",
                                         "PlusManyWithParensLeft"},
                      ExpressionTestCase{U"1+(2+3)",
                                         U"InfixExpr@0..7\n"
                                         U"  Literal@0..1\n"
                                         U"    IntLiteral@0..1 \"1\"\n"
                                         U"  Plus@1..2 \"+\"\n"
                                         U"  ParenExpr@2..7\n"
                                         U"    LeftParen@2..3 \"(\"\n"
                                         U"    InfixExpr@3..6\n"
                                         U"      Literal@3..4\n"
                                         U"        IntLiteral@3..4 \"2\"\n"
                                         U"      Plus@4..5 \"+\"\n"
                                         U"      Literal@5..6\n"
                                         U"        IntLiteral@5..6 \"3\"\n"
                                         U"    RightParen@6..7 \")\"\n",
                                         "PlusManyWithParensRight"},
                      ExpressionTestCase{U"a[0]",
                                         U"Index@0..4\n"
                                         U"  Ident@0..1\n"
                                         U"    UnquotedIdent@0..1 \"a\"\n"
                                         U"  LeftSquare@1..2 \"[\"\n"
                                         U"  Literal@2..3\n"
                                         U"    IntLiteral@2..3 \"0\"\n"
                                         U"  RightSquare@3..4 \"]\"\n",
                                         "Index"},
                      ExpressionTestCase{U"a[0 + 1]",
                                         U"Index@0..8\n"
                                         U"  Ident@0..1\n"
                                         U"    UnquotedIdent@0..1 \"a\"\n"
                                         U"  LeftSquare@1..2 \"[\"\n"
                                         U"  InfixExpr@2..7\n"
                                         U"    Literal@2..4\n"
                                         U"      IntLiteral@2..3 \"0\"\n"
                                         U"      Whitespace@3..4 \" \"\n"
                                         U"    Plus@4..5 \"+\"\n"
                                         U"    Whitespace@5..6 \" \"\n"
                                         U"    Literal@6..7\n"
                                         U"      IntLiteral@6..7 \"1\"\n"
                                         U"  RightSquare@7..8 \"]\"\n",
                                         "IndexWithExpr"}),
    [](const testing::TestParamInfo<
        ExpressionParameterizedTestFixture::ParamType>& info) {
      return info.param.test_name;
    });
TEST_P(ExpressionParameterizedTestFixture, Expressions) {
  const ExpressionTestCase& param = GetParam();
  auto lexer = Lexer(param.source);
  const std::vector<Token> tokens = lexer.Tokenize();
  auto source = TokenSource(tokens, ::IsTrivia);
  auto parser = Parser(source);

  Expr(&parser);

  auto sink = TokenSink(tokens, parser.Events(), ::IsTrivia);
  const auto [node, errors] = sink.Finish();

  const std::u32string actual =
      GreenWriter<SyntaxKind>::WriteAsU32String(node, ::ToU32String);

  EXPECT_EQ(param.expected, actual);
}
};  // namespace
