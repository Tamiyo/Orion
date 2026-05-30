#include "yuzu/Compiler/CompilePipeline.h"

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

  auto typeChecker = hir::TypeChecker(ctx);
  typeChecker.check(root);

  if (options.debugHir) {
    options.out << "=== hir ===\n"
                << hir::HirPrinter::printToString(ctx, root) << '\n';
  }

  return root;
}
} // namespace

namespace {
/// Flush every diagnostic accumulated so far to `out`. Each pipeline
/// stage calls this when it detects errors so callers see the per-stage
/// failure mode (lex error, parse error, lower error) without continuing
/// the pipeline against ill-formed input.
void flushDiagnostics(const diagnostics::DiagnosticsEngine &diagnostics,
                      const diagnostics::DiagnosticPrinter &printer,
                      llvm::raw_ostream &out) {
  for (const diagnostics::Diagnostic &d : diagnostics.getDiagnostics()) {
    printer.print(d, out);
  }
}

/// Run the lex/parse/lower passes on `source` against the supplied
/// state. Shared by both the one-shot `compile()` and the stateful
/// `Session::compile()` — the only difference between them is who
/// owns the state. Stops at the first failing stage and flushes
/// whatever diagnostics accumulated.
void runPipeline(std::u32string_view source, const CompileOptions &options,
                 diagnostics::SourceId sourceId,
                 diagnostics::DiagnosticsEngine &diagnostics,
                 const diagnostics::DiagnosticPrinter &printer,
                 hir::HirContext &hirCtx) {
  const auto tokens = lexerPass(source, options);
  if (diagnostics.hasErrors()) {
    flushDiagnostics(diagnostics, printer, options.out);
    return;
  }

  const auto syntaxRoot = parserPass(tokens, options, sourceId, diagnostics);
  if (diagnostics.hasErrors()) {
    flushDiagnostics(diagnostics, printer, options.out);
    return;
  }

  const auto *hirRoot =
      hirPass(syntaxRoot, options, sourceId, diagnostics, hirCtx);
  if (diagnostics.hasErrors()) {
    flushDiagnostics(diagnostics, printer, options.out);
    return;
  }

  (void)hirRoot;
  // codegenPass(hirRoot, hirCtx, diagnostics, sourceId, options);
}
} // namespace

void compile(std::u32string_view source, CompileOptions options) {
  // TODO: thread an explicit source name from the caller (filename for
  // files, "<adhoc>" for inline). Hardcoded as "<source>" for now so
  // diagnostics still resolve to *something*.
  diagnostics::SourceMap sources;
  diagnostics::DiagnosticsEngine diagnostics;
  const diagnostics::DiagnosticPrinter printer(sources);

  const diagnostics::SourceId sourceId =
      sources.add("<source>", std::u32string(source));

  // `hirCtx` owns the arena that backs the lowered HIR tree; it must
  // outlive every consumer of `hirRoot` (the HIR printer, codegen,
  // source-map lookups).
  hir::HirContext hirCtx(diagnostics, sourceId);

  runPipeline(source, options, sourceId, diagnostics, printer, hirCtx);
}

Session::Session(CompileOptions options)
    : options(options), sources(), diagnostics(), printer(sources),
      // SourceId is a placeholder; every `compile` call rebinds it
      // via `hirCtx.setSourceId` before any span is produced.
      hirCtx(diagnostics, diagnostics::SourceId{}) {}

void Session::compile(std::u32string_view source) {
  // Each input is registered as a fresh entry in the shared source
  // map so the printer can underline the right line. The HIR arena
  // and symbol table carry over from previous calls — that's the
  // whole point of having a session.
  static int lineCounter = 0;
  const diagnostics::SourceId sourceId = sources.add(
      "<repl:" + std::to_string(++lineCounter) + ">", std::u32string(source));
  hirCtx.setSourceId(sourceId);

  runPipeline(source, options, sourceId, diagnostics, printer, hirCtx);

  // Drop diagnostics from this input so the next one starts clean —
  // `hasErrors()` on the next compile should reflect only what that
  // compile produced, not a sticky banner from earlier.
  diagnostics.clear();
}

} // namespace yuzu
