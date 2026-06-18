#include "yuzu/Compiler/CompilePipeline.h"
#include "yuzu/Util/Unicode.h"

#include <llvm/Support/raw_ostream.h>

#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

namespace {
void usage() {
  llvm::errs() << "usage: yuzu-compile [options] <file>\n"
                  "  --debug-lexer  dump the token stream\n"
                  "  --debug-ast    dump the syntax tree\n"
                  "  --debug-hir    dump the HIR\n"
                  "  --debug-anf    dump the ANF\n"
                  "  --debug        dump all of the above\n"
                  "  --time-passes  print per-pass wall-clock timings\n"
                  "  --artifacts <dir>  write the Substrait plan into <dir>\n";
}
} // namespace

// Compile a single source file as one unit. Unlike the line-at-a-time REPL,
// the whole file is one `compile()` call, so inference flows across all its
// statements (e.g. an un-annotated `let` pinned by a later use).
int main(int argc, char **argv) {
  // Debug dumps default off (a compiler, not a REPL); flags opt in.
  yuzu::CompileOptions options{
      .debugLexer = false,
      .debugAst = false,
      .debugHir = false,
      .debugAnf = false,
  };

  const char *path = nullptr;
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg = argv[i];
    if (arg == "--debug-lexer") {
      options.debugLexer = true;
    } else if (arg == "--debug-anf") {
      options.debugAnf = true;
    } else if (arg == "--debug-ast") {
      options.debugAst = true;
    } else if (arg == "--debug-hir") {
      options.debugHir = true;
    } else if (arg == "--debug") {
      options.debugLexer = options.debugAst = options.debugHir = true;
    } else if (arg == "--time-passes") {
      options.timePasses = true;
    } else if (arg == "--artifacts") {
      if (i + 1 >= argc) {
        llvm::errs() << "yuzu-compile: --artifacts needs a directory\n";
        return 2;
      }
      options.artifactsDir = argv[++i];
    } else if (arg == "-h" || arg == "--help") {
      usage();
      return 0;
    } else if (!arg.empty() && arg.front() == '-') {
      llvm::errs() << "yuzu-compile: unknown option '" << arg << "'\n";
      usage();
      return 2;
    } else if (path == nullptr) {
      path = argv[i];
    } else {
      llvm::errs() << "yuzu-compile: unexpected argument '" << arg << "'\n";
      return 2;
    }
  }

  if (path == nullptr) {
    usage();
    return 2;
  }

  std::ifstream file(path);
  if (!file) {
    llvm::errs() << "yuzu-compile: cannot open '" << path << "'\n";
    return 1;
  }

  std::ostringstream buffer;
  buffer << file.rdbuf();
  const std::u32string source = yuzu::util::decodeUtf8(buffer.str());

  yuzu::compile(source, options);
  return 0;
}