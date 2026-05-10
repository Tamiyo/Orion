#ifndef YUZU_DIAGNOSTICS_DIAGNOSTIC_PRINTER_H
#define YUZU_DIAGNOSTICS_DIAGNOSTIC_PRINTER_H

#include "yuzu/Diagnostics/Diagnostic.h"
#include "yuzu/Diagnostics/SourceMap.h"

#include "llvm/Support/raw_ostream.h"

#include <string>

namespace yuzu::diagnostics {

/// \brief Formats a `Diagnostic` into the rustc/clang block style and
/// writes it to a stream.
///
/// \code
///   error[E0001]: expected expression
///    --> <repl>:1:3
///     |
///   1 | 2 ^ 2
///     |   ^ expected expression
///     = note: expressions can start with a number, identifier, or `(`
/// \endcode
///
/// Mirrors `syntax::SyntaxPrinter` / `syntax::GreenPrinter` — pure
/// formatting, plain text, any `llvm::raw_ostream` consumes it. There is
/// no terminal-only behaviour (no color, no width detection); the output
/// is what you get whether the destination is a tty, a log file, or a
/// gtest assertion buffer. Color/width-aware variants would layer on top
/// rather than replace this.
///
/// Spans in the diagnostic must reference `SourceId`s issued by the
/// `SourceMap` passed at construction. Single-line spans render
/// faithfully; a span that crosses a newline currently underlines only
/// the first line.
class [[nodiscard]] DiagnosticPrinter final {
public:
  /// \brief Construct a printer that resolves spans through `sources`.
  ///
  /// `sources` is borrowed for the printer's lifetime; the caller keeps
  /// ownership.
  explicit DiagnosticPrinter(const SourceMap &sources) : sources(sources) {}

  DiagnosticPrinter() = delete;

  /// \brief Format `diagnostic` and write the result to `out`.
  ///
  /// The output ends with a trailing newline so consecutive diagnostics
  /// stack cleanly.
  void print(const Diagnostic &diagnostic, llvm::raw_ostream &out) const;

  /// \brief Format `diagnostic` into a freshly-allocated string.
  ///
  /// Convenience wrapper for tests, debugging, and ad-hoc inspection
  /// that don't want to manage their own stream.
  [[nodiscard]] std::string printToString(const Diagnostic &diagnostic) const;

private:
  const SourceMap &sources;
};

} // namespace yuzu::diagnostics

#endif // YUZU_DIAGNOSTICS_DIAGNOSTIC_PRINTER_H
