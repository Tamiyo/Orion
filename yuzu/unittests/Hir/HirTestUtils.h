#ifndef YUZU_UNITTESTS_HIR_HIR_TEST_UTILS_H
#define YUZU_UNITTESTS_HIR_HIR_TEST_UTILS_H

#include "yuzu/Ast/Ast.h"
#include "yuzu/Diagnostics/DiagnosticsEngine.h"
#include "yuzu/Diagnostics/SourceMap.h"
#include "yuzu/Diagnostics/Span.h"
#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/HirBuilder.h"
#include "yuzu/Hir/HirLowerer.h"
#include "yuzu/Hir/HirSourceMap.h"
#include "yuzu/Lexer/Lexer.h"
#include "yuzu/Lexer/Token.h"
#include "yuzu/Parser/Event.h"
#include "yuzu/Parser/Grammar/Expr.h"
#include "yuzu/Parser/Grammar/Grammar.h"
#include "yuzu/Parser/Parser.h"
#include "yuzu/Parser/TokenSink.h"
#include "yuzu/Parser/TokenSource.h"

#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace yuzu::hir::test {

/// gtest fixture that owns the per-test compiler context (`SourceMap`,
/// `DiagnosticsEngine`, `HirBuilder`) and exposes a `lowerExpr` helper
/// that runs the full lex → parse → lower pipeline.
class HirFixture : public ::testing::Test {
protected:
  diagnostics::SourceMap sources;
  diagnostics::DiagnosticsEngine engine;
  HirBuilder builder;
  HirSourceMap sourceMap;

  /// Lex + parse `source` as an expression, then lower the result. Returns
  /// the HIR root expression (may be `nullptr` if lowering reported an
  /// error). Diagnostics land on `engine`.
  const Expr *lowerExpr(std::u32string_view source) {
    auto lexer = lexer::Lexer(source);
    std::vector<lexer::Token> tokens = lexer.getTokens();

    const diagnostics::SourceId sourceId =
        sources.add("<test>", std::u32string(source));

    auto parser = parser::Parser(parser::TokenSource(tokens));
    auto _ = parser::parseExpr(parser);

    std::vector<parser::Event> events = std::move(parser).finish();
    auto sink = parser::TokenSink(std::move(tokens), std::move(events), engine,
                                  sourceId);
    parser::TokenSink::Result sinkResult = sink.finish();

    const ast::Expr expr{ast::SyntaxNode::createRoot(sinkResult.green)};
    HirLowerer lowerer(builder, sourceMap, engine, sourceId);
    return lowerer.lowerExpr(expr);
  }

  /// Lex + parse `source` as a full program (sequence of statements),
  /// then lower the resulting `ast::Root` into HIR.
  const Root *lowerRoot(std::u32string_view source) {
    auto lexer = lexer::Lexer(source);
    std::vector<lexer::Token> tokens = lexer.getTokens();

    const diagnostics::SourceId sourceId =
        sources.add("<test>", std::u32string(source));

    auto parser = parser::Parser(parser::TokenSource(tokens));
    parser::parseRoot(parser);

    std::vector<parser::Event> events = std::move(parser).finish();
    auto sink = parser::TokenSink(std::move(tokens), std::move(events), engine,
                                  sourceId);
    parser::TokenSink::Result sinkResult = sink.finish();

    const ast::Root root{ast::SyntaxNode::createRoot(sinkResult.green)};
    HirLowerer lowerer(builder, sourceMap, engine, sourceId);
    return lowerer.lower(root);
  }
};

} // namespace yuzu::hir::test

#endif // YUZU_UNITTESTS_HIR_HIR_TEST_UTILS_H
