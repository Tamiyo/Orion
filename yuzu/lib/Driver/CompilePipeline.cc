#include "yuzu/Driver/CompilePipeline.h"

#include "yuzu/Ast/Ast.h"
#include "yuzu/Diagnostics/DiagnosticPrinter.h"
#include "yuzu/Diagnostics/DiagnosticsEngine.h"
#include "yuzu/Diagnostics/SourceMap.h"
#include "yuzu/Diagnostics/Span.h"
#include "yuzu/Hir/HirBuilder.h"
#include "yuzu/Hir/HirContext.h"
#include "yuzu/Hir/HirLowerer.h"
#include "yuzu/Hir/HirPrinter.h"
#include "yuzu/Hir/Types/TypeChecker.h"
#include "yuzu/Lexer/Lexer.h"
#include "yuzu/Lexer/Token.h"
#include "yuzu/Lexer/TokenKind.h"
#include "yuzu/Parser/Event.h"
#include "yuzu/Parser/Grammar/Grammar.h"
#include "yuzu/Parser/Parser.h"
#include "yuzu/Parser/TokenSink.h"
#include "yuzu/Parser/TokenSource.h"
#include "yuzu/Syntax/SyntaxPrinter.h"

#include <llvm/Support/raw_ostream.h>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace yuzu {
namespace {
std::vector<lexer::Token> lexerPass(std::u32string_view source,
                                    const CompileOptions &options) {
  auto lexer = lexer::Lexer(source);
  std::vector<lexer::Token> tokens = lexer.getTokens();

  if (options.debugLexer) {
    options.out << "=== tokens ===\n";
    for (const lexer::Token &token : tokens) {
      const auto range = token.getRange();
      options.out << lexer::asString(token.getKind()) << '@' << range.start
                  << ".." << range.end << '\n';
    }
  }

  return tokens;
}

ast::SyntaxNode parserPass(const std::vector<lexer::Token> &tokens,
                           const CompileOptions &options,
                           diagnostics::SourceId sourceId,
                           diagnostics::DiagnosticsEngine &diagnostics) {
  auto parser = parser::Parser(parser::TokenSource(tokens));
  parser::parseRoot(parser);

  std::vector<parser::Event> events = std::move(parser).finish();
  auto sink = parser::TokenSink(std::move(tokens), std::move(events),
                                diagnostics, sourceId);
  parser::TokenSink::Result sinkResult = sink.finish();

  const auto syntaxRoot = ast::SyntaxNode::createRoot(sinkResult.green);

  if (options.debugAst) {
    using SyntaxPrinter = syntax::SyntaxPrinter<ast::SyntaxKind>;
    options.out << "=== syntax ===\n"
                << SyntaxPrinter::printToString(syntaxRoot) << '\n';
  }

  return syntaxRoot;
}

const hir::Root *hirPass(ast::SyntaxNode syntaxRoot,
                         const CompileOptions &options,
                         diagnostics::SourceId sourceId,
                         diagnostics::DiagnosticsEngine &diagnostics,
                         hir::HirContext &ctx) {
  hir::HirLowerer lowerer(ctx, diagnostics, sourceId);
  const hir::Root *root = lowerer.lower(ast::Root{syntaxRoot});

  auto typeChecker = hir::TypeChecker(ctx, diagnostics, sourceId);
  typeChecker.check(root);

  if (options.debugHir) {
    options.out << "=== hir ===\n"
                << hir::HirPrinter::printToString(ctx, root) << '\n';
  }

  return root;
}
} // namespace

void compile(std::u32string_view source, CompileOptions options) {
  // TODO: thread an explicit source name from the caller (filename for
  // files, "<adhoc>" for inline). Hardcoded for now so diagnostics still
  // resolve to *something*.
  diagnostics::SourceMap sources;
  diagnostics::DiagnosticsEngine diagnostics;
  const diagnostics::SourceId sourceId =
      sources.add("<source>", std::u32string(source));

  // const owns the arena(s) that backs the lowered HIR tree, so it must
  // outlive every consumer of `hirRoot` (the HIR printer, codegen, the
  // source-map lookups).
  auto hirContext = hir::HirContext(diagnostics, sourceId);

  const auto tokens = lexerPass(source, options);
  const auto syntaxRoot = parserPass(tokens, options, sourceId, diagnostics);
  const auto *hirRoot =
      hirPass(syntaxRoot, options, sourceId, diagnostics, hirContext);

  const diagnostics::DiagnosticPrinter printer(sources);
  for (const diagnostics::Diagnostic &d : diagnostics.getDiagnostics()) {
    printer.print(d, options.out);
  }

  if (diagnostics.hasErrors()) {
    return;
  }

  (void)hirRoot;
  // codegenPass(hirRoot, hirContext, diagnostics, sourceId, options);
}

} // namespace yuzu
