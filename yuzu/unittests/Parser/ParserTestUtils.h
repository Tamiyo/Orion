#ifndef YUZU_UNITTESTS_PARSER_PARSER_TEST_UTILS_H
#define YUZU_UNITTESTS_PARSER_PARSER_TEST_UTILS_H

#include "yuzu/Ast/Ast.h"
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

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace yuzu::parser::test {

/// Snapshot of one lex → parse → sink pipeline run.
///
/// `tree` is the green tree printed via
/// `syntax::SyntaxPrinter<ast::SyntaxKind>` and is intended to be compared
/// against a raw-string literal in test assertions. `errors` mirrors
/// `TokenSink::Result::errors` so callers can assert error counts and contents
/// without re-running the pipeline.
struct ParseResult {
  std::string tree;
  std::vector<std::string> errors;
};

/// Drive the lex → parse → sink pipeline against `source` using the
/// top-level grammar entry. `parser::parseRoot` opens its own `Root`-kinded
/// marker so this helper just forwards.
inline ParseResult parseRoot(std::u32string_view source) {
  auto lexer = lexer::Lexer(source);
  std::vector<lexer::Token> tokens = lexer.getTokens();

  auto parser = Parser(TokenSource(tokens));
  parser::parseRoot(parser);

  std::vector<Event> events = std::move(parser).finish();
  auto sink = TokenSink(std::move(tokens), std::move(events));
  TokenSink::Result sinkResult = sink.finish();

  using SyntaxPrinter = syntax::SyntaxPrinter<ast::SyntaxKind>;
  return ParseResult{
      .tree = SyntaxPrinter::printToString(
          ast::SyntaxNode::createRoot(sinkResult.green)),
      .errors = std::move(sinkResult.errors),
  };
}

/// Drive the lex → parse → sink pipeline against `source` using the
/// statement grammar entry. `parser::parseStmt` expects to run inside an
/// open marker, so this helper supplies a `Stmt`-kinded one.
inline ParseResult parseStmt(std::u32string_view source) {
  auto lexer = lexer::Lexer(source);
  std::vector<lexer::Token> tokens = lexer.getTokens();

  auto parser = Parser(TokenSource(tokens));
  const Marker root = parser.start();
  parser::parseStmt(parser);
  auto _ = parser.complete(root, ast::SyntaxKind::Stmt);

  std::vector<Event> events = std::move(parser).finish();
  auto sink = TokenSink(std::move(tokens), std::move(events));
  TokenSink::Result sinkResult = sink.finish();

  using SyntaxPrinter = syntax::SyntaxPrinter<ast::SyntaxKind>;
  return ParseResult{
      .tree = SyntaxPrinter::printToString(
          ast::SyntaxNode::createRoot(sinkResult.green)),
      .errors = std::move(sinkResult.errors),
  };
}

/// Drive the lex → parse → sink pipeline against `source` using the
/// expression grammar entry. `parser::parseExpr` expects to run inside an
/// open marker, so this helper supplies an `Expr`-kinded one.
inline ParseResult parseExpr(std::u32string_view source) {
  auto lexer = lexer::Lexer(source);
  std::vector<lexer::Token> tokens = lexer.getTokens();

  auto parser = Parser(TokenSource(tokens));
  const Marker root = parser.start();
  parser::parseExpr(parser);
  auto _ = parser.complete(root, ast::SyntaxKind::Expr);

  std::vector<Event> events = std::move(parser).finish();
  auto sink = TokenSink(std::move(tokens), std::move(events));
  TokenSink::Result sinkResult = sink.finish();

  using SyntaxPrinter = syntax::SyntaxPrinter<ast::SyntaxKind>;
  return ParseResult{
      .tree = SyntaxPrinter::printToString(
          ast::SyntaxNode::createRoot(sinkResult.green)),
      .errors = std::move(sinkResult.errors),
  };
}

} // namespace yuzu::parser::test

#endif // YUZU_UNITTESTS_PARSER_PARSER_TEST_UTILS_H
