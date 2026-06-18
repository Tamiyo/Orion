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

#include <llvm/ADT/ScopeExit.h>
#include <llvm/Support/Format.h>
#include <llvm/Support/raw_ostream.h>

#include <chrono>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace yuzu {
namespace {
/// Times each compiler pass and prints an LLVM-style report when it goes out of
/// scope (so partial timings still print if a pass fails). A no-op, and prints
/// nothing, unless enabled.
class PassTimer {
public:
  PassTimer(bool enabled, llvm::raw_ostream &out)
      : enabled(enabled), out(out) {}
  PassTimer(const PassTimer &) = delete;
  PassTimer &operator=(const PassTimer &) = delete;
  ~PassTimer() {
    if (!enabled || timings.empty()) {
      return;
    }
    double total = 0.0;
    out << "=== compile-time report (ms) ===\n";
    for (const auto &[name, ms] : timings) {
      out << llvm::format("  %-12s %9.3f\n", name.c_str(), ms);
      total += ms;
    }
    out << llvm::format("  %-12s %9.3f\n", "total", total);
  }

  /// Run one pass `fn` under `name`, recording its wall time, and return
  /// whatever it returns. When disabled, just runs `fn`.
  template <typename Fn> decltype(auto) time(llvm::StringRef name, Fn &&fn) {
    if (!enabled) {
      return std::forward<Fn>(fn)();
    }
    const auto start = std::chrono::steady_clock::now();
    // Fires after `fn`'s result is materialized but before `time` returns, so
    // the recorded span is exactly `fn`'s execution.
    llvm::scope_exit record([&] {
      const std::chrono::duration<double, std::milli> elapsed =
          std::chrono::steady_clock::now() - start;
      timings.emplace_back(name.str(), elapsed.count());
    });
    return std::forward<Fn>(fn)();
  }

private:
  bool enabled;
  llvm::raw_ostream &out;
  std::vector<std::pair<std::string, double>> timings;
};

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

// Run the recursive-descent parser, producing the flat event stream.
std::vector<parser::Event>
parseEventsPass(const std::vector<lexer::Token> &tokens) {
  auto parser = parser::Parser(parser::TokenSource(tokens));
  parser::parseRoot(parser);
  return std::move(parser).finish();
}

// Replay the events into a green tree and wrap it as the red syntax root.
ast::SyntaxNode buildTreePass(const std::vector<lexer::Token> &tokens,
                              std::vector<parser::Event> events,
                              const CompileOptions &options,
                              diagnostics::SourceId sourceId,
                              diagnostics::DiagnosticsEngine &diagnostics) {
  auto sink = parser::TokenSink(std::vector<lexer::Token>(tokens),
                                std::move(events), diagnostics, sourceId);
  parser::TokenSink::Result sinkResult = sink.finish();

  const auto syntaxRoot = ast::SyntaxNode::createRoot(sinkResult.green);

  if (options.debugAst) {
    using SyntaxPrinter = syntax::SyntaxPrinter<ast::SyntaxKind>;
    options.out << "=== syntax ===\n"
                << SyntaxPrinter::printToString(syntaxRoot) << '\n';
  }

  return syntaxRoot;
}

// Lower the syntax tree to (untyped) HIR.
const hir::Root *hirLowerPass(ast::SyntaxNode syntaxRoot,
                              diagnostics::SourceId sourceId,
                              diagnostics::DiagnosticsEngine &diagnostics,
                              hir::HirContext &ctx) {
  hir::HirLowerer lowerer(ctx, diagnostics, sourceId);
  return lowerer.lower(ast::Root{syntaxRoot});
}

// Infer + concretize types over the HIR (the type-resolution pass).
void typeResolvePass(const hir::Root *root, const CompileOptions &options,
                     hir::HirContext &ctx) {
  hir::TypeResolver(ctx).resolve(root);

  if (options.debugHir) {
    options.out << "=== hir ===\n"
                << hir::HirPrinter::printToString(ctx, root) << '\n';
  }
}

const anf::Root *anfLowerPass(const hir::Root *hirRoot,
                              const CompileOptions &options,
                              diagnostics::SourceId sourceId,
                              diagnostics::DiagnosticsEngine &diagnostics,
                              anf::AnfContext &anfCtx) {
  anf::AnfLowerer lowerer(anfCtx, diagnostics, sourceId);
  const anf::Root *root = lowerer.lowerRoot(hirRoot);

  if (options.debugAnf) {
    options.out << "=== anf ===\n"
                << anf::AnfPrinter::printToString(root) << '\n';
  }

  return root;
}

void anfReducePass(const anf::Root *anfRoot, const CompileOptions &options,
                   anf::AnfContext &anfCtx) {
  anf::AnfReducer(anfCtx).reduce(anfRoot);

  if (options.debugAnf) {
    options.out << "=== anf after reduction ===\n"
                << anf::AnfPrinter::printToString(anfRoot) << '\n';
  }
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
  PassTimer timer(options.timePasses, options.out);

  const auto tokens =
      timer.time("lex", [&] { return lexerPass(source, options); });
  if (diagnostics.hasErrors()) {
    flushDiagnostics(diagnostics, printer, options.out);
    return;
  }

  auto events = timer.time("parse", [&] { return parseEventsPass(tokens); });
  const auto syntaxRoot = timer.time("build-tree", [&] {
    return buildTreePass(tokens, std::move(events), options, sourceId,
                         diagnostics);
  });
  if (diagnostics.hasErrors()) {
    flushDiagnostics(diagnostics, printer, options.out);
    return;
  }

  const auto *hirRoot = timer.time("hir-lower", [&] {
    return hirLowerPass(syntaxRoot, sourceId, diagnostics, hirCtx);
  });
  if (diagnostics.hasErrors()) {
    flushDiagnostics(diagnostics, printer, options.out);
    return;
  }

  timer.time("type-resolve",
             [&] { typeResolvePass(hirRoot, options, hirCtx); });
  if (diagnostics.hasErrors()) {
    flushDiagnostics(diagnostics, printer, options.out);
    return;
  }

  const auto *anfRoot = timer.time("anf-lower", [&] {
    return anfLowerPass(hirRoot, options, sourceId, diagnostics, anfCtx);
  });
  if (diagnostics.hasErrors()) {
    flushDiagnostics(diagnostics, printer, options.out);
    return;
  }

  timer.time("anf-reduce", [&] { anfReducePass(anfRoot, options, anfCtx); });
  if (diagnostics.hasErrors()) {
    flushDiagnostics(diagnostics, printer, options.out);
    return;
  }

  timer.time("codegen", [&] { codegenPass(anfRoot, options); });
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
  // source-map lookups). The interner is shared by both contexts and
  // must outlive them.
  util::StringInterner interner;
  hir::HirContext hirCtx(diagnostics, sourceId, interner);
  anf::AnfContext anfCtx{diagnostics, sourceId, hirCtx, interner};

  runPipeline(source, options, sourceId, diagnostics, printer, hirCtx, anfCtx);
}

Session::Session(CompileOptions options)
    : options(options), sources(), diagnostics(), printer(sources),
      // SourceId is a placeholder; every `compile` call rebinds it
      // via `hirCtx.setSourceId` before any span is produced.
      hirCtx(diagnostics, diagnostics::SourceId{}, interner),
      anfCtx(diagnostics, diagnostics::SourceId{}, hirCtx, interner) {}

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
