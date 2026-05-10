#ifndef YUZU_UNITTESTS_PARSER_PARSER_TEST_UTILS_H
#define YUZU_UNITTESTS_PARSER_PARSER_TEST_UTILS_H

#include "yuzu/Ast/Ast.h"
#include "yuzu/Diagnostics/DiagnosticsEngine.h"
#include "yuzu/Diagnostics/SourceMap.h"
#include "yuzu/Diagnostics/Span.h"
#include "yuzu/Lexer/Lexer.h"
#include "yuzu/Lexer/Token.h"
#include "yuzu/Parser/Event.h"
#include "yuzu/Parser/Grammar/Expr.h"
#include "yuzu/Parser/Grammar/Grammar.h"
#include "yuzu/Parser/Grammar/Stmt.h"
#include "yuzu/Parser/Marker.h"
#include "yuzu/Parser/Parser.h"
#include "yuzu/Parser/TokenSink.h"
#include "yuzu/Parser/TokenSource.h"
#include "yuzu/Syntax/SyntaxPrinter.h"
#include "yuzu/Util/Unicode.h"

#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace yuzu::parser::test {

/// Snapshot of one lex → parse → sink pipeline run.
///
/// `tree` is the green tree printed via
/// `syntax::SyntaxPrinter<ast::SyntaxKind>` and is intended to be
/// compared against a raw-string literal in test assertions. Diagnostics
/// live on the engine the fixture owns — tests reach for them via
/// `engine.getDiagnostics()` rather than off the result.
struct ParseResult {
  std::string tree;
};

/// \brief gtest fixture that owns the per-test compiler context (a
/// `SourceMap` and a `DiagnosticsEngine` for now; future components —
/// type tables, etc. — slot in here too) and exposes parse helpers as
/// member functions.
///
/// Inherit with `class FooTest : public yuzu::parser::test::ParserFixture`
/// and write tests as `TEST_F(FooTest, ...)`. Each test case gets a fresh
/// fixture instance, so leftover state from one test can't bleed into
/// the next. Members are `protected` so individual tests can inspect
/// them directly (e.g. `engine.hasErrors()`,
/// `engine.getDiagnostics()`).
///
/// Helpers like `parseRoot` are methods rather than free functions so
/// they pull the context (engine, source map, source id) from `this` —
/// adding a new component is a one-line change to this fixture, not an
/// update to every helper signature and call site.
class ParserFixture : public ::testing::Test {
protected:
  diagnostics::SourceMap sources;
  diagnostics::DiagnosticsEngine engine;

  /// \brief Drive the full pipeline against `source` using the top-level
  /// grammar entry. `parseRoot` opens its own `Root`-kinded marker.
  ParseResult parseRoot(std::u32string_view source) {
    return run(source, [](Parser &p) { parser::parseRoot(p); });
  }

  /// \brief Drive the pipeline against `source` using the statement
  /// grammar entry, wrapped in a `Stmt`-kinded marker.
  ParseResult parseStmt(std::u32string_view source) {
    return run(source, [](Parser &p) {
      const Marker root = p.start();
      parser::parseStmt(p);
      auto _ = p.complete(root, ast::SyntaxKind::Stmt);
    });
  }

  /// \brief Drive the pipeline against `source` using the expression
  /// grammar entry, wrapped in an `Expr`-kinded marker.
  ParseResult parseExpr(std::u32string_view source) {
    return run(source, [](Parser &p) {
      const Marker root = p.start();
      parser::parseExpr(p);
      auto _ = p.complete(root, ast::SyntaxKind::Expr);
    });
  }

private:
  /// Shared lex → parse → sink → print plumbing. The grammar callable
  /// is responsible for whatever marker bookkeeping its entry point
  /// requires; this method owns everything else, including registering
  /// the source string with the SourceMap so diagnostic spans can
  /// resolve back to a name.
  template <typename Grammar>
  ParseResult run(std::u32string_view source, Grammar grammar) {
    auto lexer = lexer::Lexer(source);
    std::vector<lexer::Token> tokens = lexer.getTokens();

    // Register the source under a synthetic `<test>` name so diagnostics
    // can reach back through the source map for line/col + snippet
    // rendering.
    const diagnostics::SourceId sourceId =
        sources.add("<test>", util::toUtf8(source));

    auto parser = Parser(TokenSource(tokens));
    grammar(parser);

    std::vector<Event> events = std::move(parser).finish();
    auto sink =
        TokenSink(std::move(tokens), std::move(events), engine, sourceId);
    TokenSink::Result sinkResult = sink.finish();

    using SyntaxPrinter = syntax::SyntaxPrinter<ast::SyntaxKind>;
    return ParseResult{
        .tree = SyntaxPrinter::printToString(
            ast::SyntaxNode::createRoot(sinkResult.green)),
    };
  }
};

} // namespace yuzu::parser::test

#endif // YUZU_UNITTESTS_PARSER_PARSER_TEST_UTILS_H
