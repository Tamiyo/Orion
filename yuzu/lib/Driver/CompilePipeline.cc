#include "yuzu/Driver/CompilePipeline.h"

#include "yuzu/Ast/Ast.h"
#include "yuzu/Codegen/HirDiagnosticHandler.h"
#include "yuzu/Codegen/HirToMlir.h"
#include "yuzu/Codegen/Jit.h"
#include "yuzu/Diagnostics/DiagnosticPrinter.h"
#include "yuzu/Diagnostics/DiagnosticsEngine.h"
#include "yuzu/Diagnostics/SourceMap.h"
#include "yuzu/Diagnostics/Span.h"
#include "yuzu/Hir/HirBuilder.h"
#include "yuzu/Hir/HirLowerer.h"
#include "yuzu/Hir/HirPrinter.h"
#include "yuzu/Hir/HirSourceMap.h"
#include "yuzu/Lexer/Lexer.h"
#include "yuzu/Lexer/Token.h"
#include "yuzu/Lexer/TokenKind.h"
#include "yuzu/Parser/Event.h"
#include "yuzu/Parser/Grammar/Grammar.h"
#include "yuzu/Parser/Parser.h"
#include "yuzu/Parser/TokenSink.h"
#include "yuzu/Parser/TokenSource.h"
#include "yuzu/Syntax/SyntaxPrinter.h"

#include "mlir/IR/MLIRContext.h"

#include "llvm/Support/raw_ostream.h"

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
                           diagnostics::DiagnosticsEngine &engine) {
  auto parser = parser::Parser(parser::TokenSource(tokens));
  parser::parseRoot(parser);

  std::vector<parser::Event> events = std::move(parser).finish();
  auto sink =
      parser::TokenSink(std::move(tokens), std::move(events), engine, sourceId);
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
                         diagnostics::DiagnosticsEngine &engine,
                         hir::HirBuilder &builder,
                         hir::HirSourceMap &sourceMap) {
  hir::HirLowerer lowerer(builder, sourceMap, engine, sourceId);
  const hir::Root *hirRoot = lowerer.lower(ast::Root{syntaxRoot});

  if (options.debugHir) {
    options.out << "=== hir ===\n"
                << hir::HirPrinter::printToString(hirRoot) << '\n';
  }

  return hirRoot;
}

void codegenPass(const hir::Root *hirRoot, const hir::HirSourceMap &sourceMap,
                 diagnostics::DiagnosticsEngine &engine,
                 diagnostics::SourceId sourceId,
                 const CompileOptions &options) {
  mlir::MLIRContext ctx;
  // Route MLIR diagnostics back through our engine so any error fired
  // during lowering / JIT lands on the original HIR's source span.
  codegen::installHirDiagnosticHandler(ctx, sourceMap, sourceId, engine);

  auto module = codegen::lowerHirToMlir(ctx, hirRoot);
  if (!module) {
    return;
  }

  if (options.debugMlir) {
    options.out << "=== mlir ===\n";
    module->print(options.out);
    options.out << '\n';
  }

  if (!options.execute) {
    return;
  }

  // `jitExecute` consumes the module via the pass pipeline (lowering it
  // to LLVM dialect in place), so any MLIR dump above must happen first.
  const auto result = codegen::jitExecute(ctx, *module);
  options.out << "=== result ===\n";
  if (result) {
    options.out << *result << '\n';
  } else {
    options.out << "(execution failed)\n";
  }
}

} // namespace

void compile(std::u32string_view source, CompileOptions options) {
  // TODO: thread an explicit source name from the caller (filename for
  // files, "<adhoc>" for inline). Hardcoded for now so diagnostics still
  // resolve to *something*.
  diagnostics::SourceMap sources;
  diagnostics::DiagnosticsEngine engine;
  const diagnostics::SourceId sourceId =
      sources.add("<source>", std::u32string(source));

  // HirBuilder owns the arena that backs the lowered HIR tree, so it must
  // outlive every consumer of `hirRoot` (the HIR printer, codegen, the
  // source-map lookups). Declare both alongside the other long-lived
  // context here.
  hir::HirBuilder builder;
  hir::HirSourceMap sourceMap;

  const auto tokens = lexerPass(source, options);
  const auto syntaxRoot = parserPass(tokens, options, sourceId, engine);
  const auto *hirRoot =
      hirPass(syntaxRoot, options, sourceId, engine, builder, sourceMap);

  const diagnostics::DiagnosticPrinter printer(sources);
  for (const diagnostics::Diagnostic &d : engine.getDiagnostics()) {
    printer.print(d, options.out);
  }

  if (engine.hasErrors()) {
    return;
  }

  codegenPass(hirRoot, sourceMap, engine, sourceId, options);
}

} // namespace yuzu
