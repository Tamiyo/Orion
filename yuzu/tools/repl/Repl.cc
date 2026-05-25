#include "yuzu/Compiler/CompilePipeline.h"
#include "yuzu/Util/Unicode.h"

#include <llvm/Support/raw_ostream.h>

#include <iostream>
#include <string>

// Line-at-a-time REPL with persistent state. A single `yuzu::Session`
// owns the HIR arena, symbol table, type interner, and source map,
// so a `let` binding entered on one line is visible on the next.
// Diagnostics are flushed and cleared at the end of every compile so
// errors don't bleed forward. EOF (Ctrl+D) exits cleanly.
int main() {
  llvm::outs() << "yuzu repl — Ctrl+D to exit\n";

  yuzu::Session session;

  std::string line;
  while (true) {
    llvm::outs() << "> ";
    llvm::outs().flush();

    if (!std::getline(std::cin, line)) {
      llvm::outs() << '\n';
      break;
    }

    if (line.empty()) {
      continue;
    }

    // The decoded source must outlive the lexer (which borrows it as
    // a string_view). The session's source map will also retain its
    // own copy, but we keep this local one alive for the duration of
    // the call to avoid an extra round-trip.
    const std::u32string source = yuzu::util::decodeUtf8(line);
    session.compile(source);
  }

  return 0;
}
