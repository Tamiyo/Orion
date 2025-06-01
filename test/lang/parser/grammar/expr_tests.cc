#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

#include "ApprovalTests/ApprovalTests.hpp"
#include "lang/lexer/lexer.h"
#include "lang/lexer/token.h"
#include "lang/lexer/token_kind.h"
#include "lang/parser/grammar/expr.h"
#include "lang/parser/parser.h"
#include "lang/parser/syntax_kind.h"
#include "lang/parser/token_sink.h"
#include "lang/parser/token_source.h"
#include "syntax/rgtree/green_writer.h"
#include "utils/strings.h"

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

struct ExprTestCase {
  const std::u32string source;
  const std::string test_name;
};

class ExprParameterizedTestFixture
    : public testing::TestWithParam<ExprTestCase> {};

void TextExprParses(const std::u32string& input) {
  auto lexer = Lexer(input);
  const std::vector<Token> tokens = lexer.Tokenize();
  auto source = TokenSource(tokens, ::IsTrivia);
  auto parser = Parser(source);

  Expr(&parser);

  auto sink = TokenSink(tokens, parser.Events(), ::IsTrivia);
  const auto [node, errors] = sink.Finish();

  const std::u32string actual =
      GreenWriter<SyntaxKind>::WriteAsU32String(node, ::ToU32String);

  ApprovalTests::Approvals::verify(yuzu::utils::U32ToU8(actual));
}

INSTANTIATE_TEST_SUITE_P(
    BasicLhs, ExprParameterizedTestFixture,
    ::testing::Values(ExprTestCase{U"abc", "UnquotedIdent"},
                      ExprTestCase{U"`abc`", "QuotedIdent"},
                      ExprTestCase{U"true", "BooleanLit"},
                      ExprTestCase{U"\"hello world\"", "StringLit"},
                      ExprTestCase{U"1337BD", "BigDecimalLit"},
                      ExprTestCase{U"1337L", "BigIntLit"},
                      ExprTestCase{U"1337", "IntLit"},
                      ExprTestCase{U"1337S", "SmallIntLit"},
                      ExprTestCase{U"137Y", "TinyIntLit"},
                      ExprTestCase{U"1.34F", "FloatLit"},
                      ExprTestCase{U"1.34D", "DoubleLit"}),
    [](const testing::TestParamInfo<ExprParameterizedTestFixture::ParamType>&
           info) { return info.param.test_name; });
TEST_P(ExprParameterizedTestFixture, ParseBasicLhsExprs) {
  const auto& [source, _] = GetParam();
  TextExprParses(source);
}

INSTANTIATE_TEST_SUITE_P(
    ComplexLhs, ExprParameterizedTestFixture,
    ::testing::Values(ExprTestCase{U"+ 1", "AddPrefix"},
                      ExprTestCase{U"- 1", "SubPrefix"},
                      ExprTestCase{U"(1)", "ParenExpr"},
                      ExprTestCase{U"(1 + 1)", "ParenExprWithAdd"},
                      ExprTestCase{U"((1))", "ParenExprNested"},
                      ExprTestCase{U"(( 1) )", "ParenExprNestedWhitespace"}),
    [](const testing::TestParamInfo<ExprParameterizedTestFixture::ParamType>&
           info) { return info.param.test_name; });
TEST_P(ExprParameterizedTestFixture, ParseComplexLhsExprs) {
  const auto& [source, _] = GetParam();
  TextExprParses(source);
}

INSTANTIATE_TEST_SUITE_P(
    BasicExprs, ExprParameterizedTestFixture,
    ::testing::Values(
        ExprTestCase{U"1 + 2", "Add"}, ExprTestCase{U"1 - 2", "Sub"},
        ExprTestCase{U"1 * 2", "Mul"}, ExprTestCase{U"1 / 2", "Div"},
        ExprTestCase{U"1 % 2", "Mod"}, ExprTestCase{U"a[0]", "Index"}),
    [](const testing::TestParamInfo<ExprParameterizedTestFixture::ParamType>&
           info) { return info.param.test_name; });
TEST_P(ExprParameterizedTestFixture, ParseBasicExprs) {
  const auto& [source, _] = GetParam();
  TextExprParses(source);
}

INSTANTIATE_TEST_SUITE_P(
    ComplexExprs, ExprParameterizedTestFixture,
    ::testing::Values(ExprTestCase{U"1 + 2 + 3", "AddMany"},
                      ExprTestCase{U"1 - 2 - 3", "SubMany"},
                      ExprTestCase{U"1 * 2 * 3", "MulMany"},
                      ExprTestCase{U"1 / 2 / 3", "DivMany"},
                      ExprTestCase{U"1 % 2 % 3", "ModMany"},

                      ExprTestCase{U"1 * 2 / 3", "MulManyPrecedenceLeft"},
                      ExprTestCase{U"1 / 2 + 3", "DivManyPrecedenceLef"},
                      ExprTestCase{U"1 % 2 + 3", "ModManyPrecedenceLeft"},

                      ExprTestCase{U"1 + 2 * 3", "MulManyPrecedenceRight"},
                      ExprTestCase{U"1 + 2 / 3", "DivManyPrecedenceRight"},
                      ExprTestCase{U"1 + 2 % 3", "ModManyPrecedenceRight"},

                      ExprTestCase{U"(1 + 2) * 3", "MulManyPrecedenceParens"},
                      ExprTestCase{U"(1 + 2) / 3", "DivManyPrecedenceParens"},
                      ExprTestCase{U"(1 + 2) % 3", "ModManyPrecedenceParens"},

                      ExprTestCase{U"a[0 + 1]", "IndexWithExpr"}),
    [](const testing::TestParamInfo<ExprParameterizedTestFixture::ParamType>&
           info) { return info.param.test_name; });
TEST_P(ExprParameterizedTestFixture, ParseComplexExprs) {
  const auto& [source, _] = GetParam();
  TextExprParses(source);
}
};  // namespace
