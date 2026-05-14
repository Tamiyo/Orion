#include "yuzu/Driver/CompilePipeline.h"
#include "yuzu/Util/Unicode.h"

#include "llvm/Support/raw_ostream.h"

#include <iostream>
#include <string>

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

    // The decoded source must outlive the lexer (which borrows it as a
    // string_view), so bind it locally before passing it down.
    const std::u32string source = yuzu::util::decodeUtf8(line);
    yuzu::compile(source);
  }

  return 0;
}
