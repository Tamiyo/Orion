#ifndef YUZU_DRIVER_COMPILE_PIPELINE_H
#define YUZU_DRIVER_COMPILE_PIPELINE_H

#include <string_view>

#include "llvm/Support/raw_ostream.h"

namespace yuzu {
struct CompileOptions {
  // Opts
  llvm::raw_ostream &out = llvm::outs();
  bool execute = true;

  // Debug
  bool debugLexer = true;
  bool debugAst = true;
  bool debugHir = true;
  bool debugMlir = true;
};

void compile(std::u32string_view source, CompileOptions options = {});
} // namespace yuzu

#endif
