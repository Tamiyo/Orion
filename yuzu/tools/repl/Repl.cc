#include "yuzu/Ast/Ast.h"
#include "yuzu/Diagnostics/DiagnosticPrinter.h"
#include "yuzu/Diagnostics/DiagnosticsEngine.h"
#include "yuzu/Diagnostics/SourceMap.h"
#include "yuzu/Diagnostics/Span.h"
#include "yuzu/Lexer/Lexer.h"
#include "yuzu/Lexer/Token.h"
#include "yuzu/Parser/Event.h"
#include "yuzu/Parser/Grammar/Grammar.h"
#include "yuzu/Parser/Parser.h"
#include "yuzu/Parser/TokenSink.h"
#include "yuzu/Parser/TokenSource.h"
#include "yuzu/Syntax/SyntaxPrinter.h"
#include "yuzu/Util/Unicode.h"

#include "llvm/Support/raw_ostream.h"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

/// Run one REPL line through the full pipeline and write the printed
/// tree to `out`, followed by any diagnostics rendered in the rustc-style
/// block format.
void compileAndPrint(std::u32string_view source, llvm::raw_ostream &out) {
  // Fresh per-line context. A future polish (multi-line history,
  // persistent diagnostics) would lift these out of the function.
  yuzu::diagnostics::SourceMap sources;
  yuzu::diagnostics::DiagnosticsEngine engine;
  const yuzu::diagnostics::SourceId sourceId =
      sources.add("<repl>", std::u32string(source));

  auto lexer = yuzu::lexer::Lexer(source);
  std::vector<yuzu::lexer::Token> tokens = lexer.getTokens();

  auto parser = yuzu::parser::Parser(yuzu::parser::TokenSource(tokens));
  yuzu::parser::parseRoot(parser);

  std::vector<yuzu::parser::Event> events = std::move(parser).finish();
  auto sink = yuzu::parser::TokenSink(std::move(tokens), std::move(events),
                                      engine, sourceId);
  yuzu::parser::TokenSink::Result result = sink.finish();

  using SyntaxPrinter = yuzu::syntax::SyntaxPrinter<yuzu::ast::SyntaxKind>;
  out << SyntaxPrinter::printToString(
             yuzu::ast::SyntaxNode::createRoot(result.green))
      << '\n';

  // Render each diagnostic as a Rust-style block: severity header, arrow
  // line, snippet, caret. The printer pulls the snippet text and source
  // name straight from `sources`.
  const yuzu::diagnostics::DiagnosticPrinter printer(sources);
  for (const yuzu::diagnostics::Diagnostic &d : engine.getDiagnostics()) {
    printer.print(d, out);
  }
}

} // namespace

int main() {
  llvm::outs() << "yuzu repl — Ctrl+D to exit\n";
  std::string line;
  while (true) {
    llvm::outs() << "> ";
    llvm::outs().flush();

    if (!std::getline(std::cin, line)) {
      // EOF (Ctrl+D) or read error — exit cleanly.
      llvm::outs() << '\n';
      break;
    }

    if (line.empty()) {
      continue;
    }

    // The line must outlive the lexer (which holds it as a string_view),
    // so bind it to a local variable here rather than passing a temporary.
    const std::u32string source = yuzu::util::decodeUtf8(line);
    compileAndPrint(source, llvm::outs());
  }

  return 0;
}
