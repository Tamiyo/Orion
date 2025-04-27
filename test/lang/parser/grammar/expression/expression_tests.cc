#include <gtest/gtest.h>

#include <codecvt>
#include <locale>
#include <string>
#include <utility>
#include <vector>

#include "ApprovalTests/ApprovalTests.hpp"
#include "lang/lexer/lexer.h"
#include "lang/lexer/token.h"
#include "lang/lexer/token_kind.h"
#include "lang/parser/grammar/expression/expression.h"
#include "lang/parser/parser.h"
#include "lang/parser/syntax_kind.h"
#include "lang/parser/token_sink.h"
#include "lang/parser/token_source.h"
#include "syntax/parser/rgtree/green/green_writer.h"
#include "utils/string_utils.h"

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
  const std::u32string source;
  const std::string test_name;
};

class ExpressionParameterizedTestFixture
    : public testing::TestWithParam<ExpressionTestCase> {};

void TextExpressionParses(const std::u32string& input) {
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
    BasicExpressions, ExpressionParameterizedTestFixture,
    ::testing::Values(ExpressionTestCase{U"1 + 2", "Plus"},
                      ExpressionTestCase{U"1 - 2", "Minus"},
                      ExpressionTestCase{U"a[0]", "Index"}),
    [](const testing::TestParamInfo<
        ExpressionParameterizedTestFixture::ParamType>& info) {
      return info.param.test_name;
    });
TEST_P(ExpressionParameterizedTestFixture, ParseBasicExpressions) {
  const ExpressionTestCase& param = GetParam();
  TextExpressionParses(param.source);
}

INSTANTIATE_TEST_SUITE_P(
    ComplexExpressions, ExpressionParameterizedTestFixture,
    ::testing::Values(ExpressionTestCase{U"1 + 2 + 3", "PlusMany"},
                      ExpressionTestCase{U"1 - 2 - 3", "MinusMany"},
                      ExpressionTestCase{U"a[0 + 1]", "IndexWithExpr"}),
    [](const testing::TestParamInfo<
        ExpressionParameterizedTestFixture::ParamType>& info) {
      return info.param.test_name;
    });
TEST_P(ExpressionParameterizedTestFixture, ParseComplexExpressions) {
  const ExpressionTestCase& param = GetParam();
  TextExpressionParses(param.source);
}
};  // namespace
