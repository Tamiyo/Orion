#ifndef YUZU_DIAGNOSTICS_DIAGNOSTIC_BUILDER_H
#define YUZU_DIAGNOSTICS_DIAGNOSTIC_BUILDER_H

#include "yuzu/Diagnostics/Diagnostic.h"
#include "yuzu/Diagnostics/Span.h"

#include <string>
#include <utility>

namespace yuzu::diagnostics {

class DiagnosticsEngine;

/// \brief Fluent builder for constructing a `Diagnostic` and handing it
/// to a `DiagnosticsEngine`.
///
/// Created by `DiagnosticsEngine::error` / `::warning` / `::remark` —
/// callers don't construct one directly. The diagnostic is mutated by
/// chained calls and finally pushed to the engine via `emit`. Forgetting
/// to call `emit` silently drops the diagnostic, which is intentional:
/// it lets recovery code abandon a half-built diagnostic without
/// special-casing it.
///
/// \code
///   engine.error(span, "expected expression")
///       .code("E0001")
///       .label(prevSpan, "this `+` needs a right-hand side")
///       .note("expressions can start with a number, identifier, or `(`")
///       .emit();
/// \endcode
class [[nodiscard]] DiagnosticBuilder final {
public:
  // Constructed by the engine.
  friend class DiagnosticsEngine;

  /// Default-constructed only via friend access; users go through the
  /// engine's `error`/`warning`/`remark` methods.
  DiagnosticBuilder() = delete;

  // Movable but not copyable: the in-flight diagnostic is owned by one
  // builder, and copying would mean accidentally double-emitting.
  DiagnosticBuilder(DiagnosticBuilder &&) = default;
  DiagnosticBuilder &operator=(DiagnosticBuilder &&) = default;
  DiagnosticBuilder(const DiagnosticBuilder &) = delete;
  DiagnosticBuilder &operator=(const DiagnosticBuilder &) = delete;

  ~DiagnosticBuilder() = default;

  /// \brief Set or replace the diagnostic code (e.g. `"E0001"`).
  DiagnosticBuilder &code(std::string code) {
    diag.code = std::move(code);
    return *this;
  }

  /// \brief Add a primary label — the focal point of the diagnostic.
  ///
  /// The first call from `DiagnosticsEngine::error` already adds a
  /// primary label using `(span, message)`; this is for the rare case of
  /// promoting an additional span to primary status (renderers will
  /// underline both with the same emphasis).
  DiagnosticBuilder &primaryLabel(Span span, std::string message = "") {
    diag.labels.push_back(Label{LabelStyle::Primary, span, std::move(message)});
    return *this;
  }

  /// \brief Add a secondary label — supporting context, e.g. "this
  /// earlier `+` is the operator that needs the RHS".
  DiagnosticBuilder &label(Span span, std::string message = "") {
    diag.labels.push_back(
        Label{LabelStyle::Secondary, span, std::move(message)});
    return *this;
  }

  /// \brief Append a plain-text note. Renders as a `note: ...` line
  /// beneath the diagnostic; carries no span.
  DiagnosticBuilder &note(std::string text) {
    diag.notes.push_back(std::move(text));
    return *this;
  }

  /// \brief Push the assembled diagnostic into the engine.
  ///
  /// Moves the in-progress diagnostic out of the builder. The builder is
  /// then in a valid-but-empty state; calling `emit` again would push an
  /// empty diagnostic, so just don't. The pre-`emit` `Diagnostic` is
  /// accessible via `peek()` for tests that want to inspect it without
  /// committing.
  void emit();

  /// \brief Read-only access to the in-progress diagnostic.
  ///
  /// Useful for unit tests that build a diagnostic and assert against
  /// its fields without going through `emit`.
  [[nodiscard]] const Diagnostic &peek() const { return diag; }

private:
  /// Internal constructor used by `DiagnosticsEngine`. Seeds the builder
  /// with a primary label spanning `span` with `message`.
  DiagnosticBuilder(DiagnosticsEngine *engine, Severity severity, Span span,
                    std::string message)
      : engine(engine) {
    diag.severity = severity;
    diag.message = std::move(message);
    diag.labels.push_back(Label{LabelStyle::Primary, span, ""});
  }

  DiagnosticsEngine *engine;
  Diagnostic diag;
};

} // namespace yuzu::diagnostics

#endif // YUZU_DIAGNOSTICS_DIAGNOSTIC_BUILDER_H
