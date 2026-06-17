#ifndef YUZU_COMPILER_COMPILE_PIPELINE_H
#define YUZU_COMPILER_COMPILE_PIPELINE_H

#include "yuzu/Anf/AnfContext.h"
#include "yuzu/Diagnostics/DiagnosticPrinter.h"
#include "yuzu/Diagnostics/DiagnosticsEngine.h"
#include "yuzu/Diagnostics/SourceMap.h"
#include "yuzu/Hir/HirContext.h"
#include "yuzu/Util/StringInterner.h"

#include <llvm/Support/raw_ostream.h>

#include <optional>
#include <string>
#include <string_view>

namespace yuzu {
struct CompileOptions {
  // Opts
  llvm::raw_ostream &out = llvm::outs();
  bool execute = true;

  // Debug (all off by default)
  bool debugLexer = false;
  bool debugAst = false;
  bool debugHir = false;
  bool debugAnf = false;

  // Directory to write build artifacts (the Substrait plan) into. Unset
  // disables codegen — no JSON is written.
  std::optional<std::string> artifactsDir = std::nullopt;
};

/// One-shot compile. Builds fresh diagnostics + HIR state, runs the
/// pipeline, prints any diagnostics, drops the state. Suitable for
/// batch compiles and tests where each input is independent.
void compile(std::u32string_view source, CompileOptions options = {});

/// Stateful compile driver. The HIR arena, symbol table, source map,
/// and type interner persist across `compile` calls, so a `let`
/// binding from one input is visible to subsequent ones. Diagnostics
/// are flushed and cleared at the end of every `compile` call, so
/// errors don't pile up.
///
/// Intended for line-at-a-time REPL use; not thread-safe.
class Session final {
public:
  explicit Session(CompileOptions options = {});

  Session(const Session &) = delete;
  Session &operator=(const Session &) = delete;
  Session(Session &&) = delete;
  Session &operator=(Session &&) = delete;

  /// Compile one input against the persistent state, registering it
  /// as a new entry in the source map so diagnostics can point at the
  /// right line.
  void compile(std::u32string_view source);

private:
  CompileOptions options;
  diagnostics::SourceMap sources;
  diagnostics::DiagnosticsEngine diagnostics;
  diagnostics::DiagnosticPrinter printer;
  // The single interner shared by both contexts; declared first so it
  // outlives them.
  util::StringInterner interner;
  // Default-constructed `SourceId` is the invalid sentinel; each
  // `compile` call rebinds it via `hirCtx.setSourceId` before any
  // span is produced.
  hir::HirContext hirCtx;
  anf::AnfContext anfCtx;
};
} // namespace yuzu

#endif
