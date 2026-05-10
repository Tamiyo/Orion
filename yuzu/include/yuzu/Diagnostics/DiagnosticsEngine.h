#ifndef YUZU_DIAGNOSTICS_DIAGNOSTICS_ENGINE_H
#define YUZU_DIAGNOSTICS_DIAGNOSTICS_ENGINE_H

#include "yuzu/Diagnostics/Diagnostic.h"
#include "yuzu/Diagnostics/DiagnosticBuilder.h"
#include "yuzu/Diagnostics/Span.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace yuzu::diagnostics {

/// \brief Collects diagnostics produced during a single compilation.
///
/// Components that can fail (lexer, parser, future semantic passes) hold
/// a reference to one of these and emit through the builder API. The
/// engine itself does no rendering — it just stores diagnostics in
/// emission order and tracks counts. A renderer walks the stored list at
/// the end.
///
/// \code
///   DiagnosticsEngine engine;
///   engine.error(span, "expected expression")
///       .label(otherSpan, "after this `+`")
///       .emit();
///
///   if (engine.hasErrors()) { ... }
/// \endcode
class [[nodiscard]] DiagnosticsEngine final {
public:
  /// \brief Begin building an `Error` diagnostic.
  ///
  /// `span` becomes the diagnostic's primary span (no inline label —
  /// add one with `.label(...)` if needed). `message` is the top-level
  /// summary printed on the first line.
  DiagnosticBuilder error(Span span, std::string message) {
    return DiagnosticBuilder(this, Severity::Error, span, std::move(message));
  }

  /// \brief Begin building a `Warning` diagnostic. Same shape as
  /// `error`; doesn't count toward `hasErrors`.
  DiagnosticBuilder warning(Span span, std::string message) {
    return DiagnosticBuilder(this, Severity::Warning, span, std::move(message));
  }

  /// \brief Begin building a `Remark` diagnostic. Lowest severity;
  /// purely informational.
  DiagnosticBuilder remark(Span span, std::string message) {
    return DiagnosticBuilder(this, Severity::Remark, span, std::move(message));
  }

  /// \brief Push a fully-built diagnostic. Normally called by
  /// `DiagnosticBuilder::emit`; exposed publicly for callers that already
  /// have a `Diagnostic` in hand (tests, derived constructors).
  void push(Diagnostic diagnostic) {
    diagnostics.push_back(std::move(diagnostic));
  }

  /// \brief Read-only access to all pushed diagnostics, in emission order.
  [[nodiscard]] const std::vector<Diagnostic> &getDiagnostics() const {
    return diagnostics;
  }

  /// \brief Count of diagnostics with `Severity::Error`.
  [[nodiscard]] size_t getErrorCount() const {
    size_t count = 0;
    for (const Diagnostic &d : diagnostics) {
      if (d.severity == Severity::Error) {
        count += 1;
      }
    }
    return count;
  }

  /// \brief Count of diagnostics with `Severity::Warning`.
  [[nodiscard]] size_t getWarningCount() const {
    size_t count = 0;
    for (const Diagnostic &d : diagnostics) {
      if (d.severity == Severity::Warning) {
        count += 1;
      }
    }
    return count;
  }

  /// \brief Convenience predicate equivalent to `getErrorCount() > 0`.
  [[nodiscard]] bool hasErrors() const { return getErrorCount() > 0; }

private:
  std::vector<Diagnostic> diagnostics;
};

} // namespace yuzu::diagnostics

#endif // YUZU_DIAGNOSTICS_DIAGNOSTICS_ENGINE_H
