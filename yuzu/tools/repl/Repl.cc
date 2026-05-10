#include "yuzu/Ast/Ast.h"
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

/// Drive `parseRoot` over `source` and write the printed tree (followed by
/// any parser errors, indented) to `out`.
void compileAndPrint(std::u32string_view source, llvm::raw_ostream &out) {
  auto lexer = yuzu::lexer::Lexer(source);
  std::vector<yuzu::lexer::Token> tokens = lexer.getTokens();

  auto parser = yuzu::parser::Parser(yuzu::parser::TokenSource(tokens));
  yuzu::parser::parseRoot(parser);

  std::vector<yuzu::parser::Event> events = std::move(parser).finish();
  auto sink = yuzu::parser::TokenSink(std::move(tokens), std::move(events));
  yuzu::parser::TokenSink::Result result = sink.finish();

  using SyntaxPrinter = yuzu::syntax::SyntaxPrinter<yuzu::ast::SyntaxKind>;
  out << SyntaxPrinter::printToString(
             yuzu::ast::SyntaxNode::createRoot(result.green))
      << '\n';

  if (!result.errors.empty()) {
    out << "errors:\n";
    for (const std::string &error : result.errors) {
      out << "  " << error << '\n';
    }
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

    // The line `decodeUtf8` returns must outlive the lexer (which holds
    // it as a string_view), so bind it to a local variable here rather
    // than passing the temporary.
    const std::u32string source = yuzu::util::decodeUtf8(line);
    compileAndPrint(source, llvm::outs());
  }

  return 0;
}
