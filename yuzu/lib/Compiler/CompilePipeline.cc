#include "yuzu/Compiler/CompilePipeline.h"

#include "yuzu/Anf/Anf.h"
#include "yuzu/Anf/AnfContext.h"
#include "yuzu/Anf/AnfLowerer.h"
#include "yuzu/Anf/AnfPrinter.h"
#include "yuzu/Anf/Reduction/AnfReducer.h"
#include "yuzu/Ast/Ast.h"
#include "yuzu/Diagnostics/DiagnosticPrinter.h"
#include "yuzu/Diagnostics/DiagnosticsEngine.h"
#include "yuzu/Diagnostics/SourceMap.h"
#include "yuzu/Diagnostics/Span.h"
#include "yuzu/Hir/HirBuilder.h"
#include "yuzu/Hir/HirContext.h"
#include "yuzu/Hir/HirLowerer.h"
#include "yuzu/Hir/HirPrinter.h"
#include "yuzu/Hir/Types/TypeResolver.h"
#include "yuzu/Lexer/Lexer.h"
#include "yuzu/Lexer/Token.h"
#include "yuzu/Lexer/TokenKind.h"
#include "yuzu/Parser/Event.h"
#include "yuzu/Parser/Grammar/Grammar.h"
#include "yuzu/Parser/Parser.h"
#include "yuzu/Parser/TokenSink.h"
#include "yuzu/Parser/TokenSource.h"
#include "yuzu/Substrait/SubstraitEmitter.h"
#include "yuzu/Syntax/SyntaxPrinter.h"

#include <llvm/Support/raw_ostream.h>

#include <string>
#include <string_view>
#include <system_error>
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

  hir::TypeResolver(ctx).resolve(root);

  if (options.debugHir) {
    options.out << "=== hir ===\n"
                << hir::HirPrinter::printToString(ctx, root) << '\n';
  }

  return root;
}

const anf::Root *anfPass(const hir::Root *hirRoot,
                         const CompileOptions &options,
                         diagnostics::SourceId sourceId,
                         diagnostics::DiagnosticsEngine &diagnostics,
                         anf::AnfContext &anfCtx, hir::HirContext &hirCtx) {
  anf::AnfLowerer lowerer(anfCtx, hirCtx, diagnostics, sourceId);
  const anf::Root *root = lowerer.lowerRoot(hirRoot);

  if (options.debugAnf) {
    options.out << "=== anf ===\n"
                << anf::AnfPrinter::printToString(root) << '\n';
  }

  anf::AnfReducer reducer(anfCtx);
  reducer.reduce(root);

  if (options.debugAnf) {
    options.out << "=== anf after reduction ===\n"
                << anf::AnfPrinter::printToString(root) << '\n';
  }

  return root;
}

/// Emit the reduced ANF as a Substrait plan into the artifacts directory.
/// A no-op unless `artifactsDir` is set; writes `<dir>/plan.substrait.json`
/// (skipped when the program has no query, so the plan is empty).
void codegenPass(const anf::Root *anfRoot, const CompileOptions &options) {
  if (!options.artifactsDir) {
    return;
  }
  const std::string plan = substrait::SubstraitEmitter().emit(anfRoot);
  if (plan.empty()) {
    return;
  }
  const std::string path = *options.artifactsDir + "/plan.substrait.json";
  std::error_code ec;
  llvm::raw_fd_ostream os(path, ec);
  if (ec) {
    options.out << "yuzu: cannot write " << path << ": " << ec.message()
                << '\n';
    return;
  }
  os << plan << '\n';
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
                 hir::HirContext &hirCtx, anf::AnfContext &anfCtx) {
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

  const auto anfRoot =
      anfPass(hirRoot, options, sourceId, diagnostics, anfCtx, hirCtx);
  if (diagnostics.hasErrors()) {
    flushDiagnostics(diagnostics, printer, options.out);
    return;
  }

  codegenPass(anfRoot, options);
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
  anf::AnfContext anfCtx{diagnostics, sourceId, hirCtx};

  runPipeline(source, options, sourceId, diagnostics, printer, hirCtx, anfCtx);
}

Session::Session(CompileOptions options)
    : options(options), sources(), diagnostics(), printer(sources),
      // SourceId is a placeholder; every `compile` call rebinds it
      // via `hirCtx.setSourceId` before any span is produced.
      hirCtx(diagnostics, diagnostics::SourceId{}),
      anfCtx(diagnostics, diagnostics::SourceId{}, hirCtx) {}

void Session::compile(std::u32string_view source) {
  // Each input is registered as a fresh entry in the shared source
  // map so the printer can underline the right line. The HIR arena
  // and symbol table carry over from previous calls — that's the
  // whole point of having a session.
  static int lineCounter = 0;
  const diagnostics::SourceId sourceId = sources.add(
      "<repl:" + std::to_string(++lineCounter) + ">", std::u32string(source));
  hirCtx.setSourceId(sourceId);

  runPipeline(source, options, sourceId, diagnostics, printer, hirCtx, anfCtx);

  // Drop diagnostics from this input so the next one starts clean —
  // `hasErrors()` on the next compile should reflect only what that
  // compile produced, not a sticky banner from earlier.
  diagnostics.clear();
}

} // namespace yuzu
