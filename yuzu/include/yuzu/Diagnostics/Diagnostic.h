#ifndef YUZU_DIAGNOSTICS_DIAGNOSTIC_H
#define YUZU_DIAGNOSTICS_DIAGNOSTIC_H

#include "yuzu/Diagnostics/Span.h"

#include <cstdint>
#include <string>
#include <vector>

namespace yuzu::diagnostics {

/// \brief Top-level severity of a diagnostic.
///
/// `Note` and `Help` aren't first-class severities here — they're
/// modelled as plain-text annotations attached to a parent diagnostic via
/// `Diagnostic::notes`. That keeps the surface narrow and matches what
/// most renderers do (a `note: ...` line beneath the parent error).
enum class Severity : uint8_t {
  /// Stops a successful build. Counted by
  /// `DiagnosticsEngine::getErrorCount`.
  Error,
  /// Compiler observation that doesn't stop the build but is informational.
  Remark,
  /// Doesn't stop the build but is surfaced to the user.
  Warning,
};

/// \brief Distinguishes the focal point of a diagnostic from supporting
/// context.
///
/// A diagnostic typically has exactly one `Primary` label (where the
/// problem actually is) and zero or more `Secondary` labels for related
/// context — e.g. "this earlier `+` is the operator that needs the RHS".
/// Renderers usually draw primary labels with a stronger underline than
/// secondaries.
enum class LabelStyle : uint8_t {
  Primary,
  Secondary,
};

/// \brief A span attached to a diagnostic, optionally with inline text.
///
/// `message` is the short text rendered next to the underline ("expected
/// expression"). It can be empty when the span itself is enough context.
struct [[nodiscard]] Label final {
  LabelStyle style;
  Span span;
  std::string message;
};

/// \brief A complete diagnostic: severity, optional code, top-level
/// message, labels pinned to source spans, and plain-text notes.
///
/// Construction is normally done through `DiagnosticBuilder` rather than
/// by direct aggregate initialization. The struct is exposed publicly so
/// renderers and tests can inspect it.
///
/// Conventions:
///
///   - `code` is optional; non-empty values look like `"E0001"` and are
///     rendered as `error[E0001]: ...`.
///   - `labels` should contain exactly one `Primary` (the focal location).
///     Secondary labels follow.
///   - `notes` are standalone "note:" / "help:" lines that don't pin to
///     a span. Use a `Secondary` label if you do want a span.
struct [[nodiscard]] Diagnostic final {
  Severity severity = Severity::Error;
  std::string code;
  std::string message;
  std::vector<Label> labels;
  std::vector<std::string> notes;
};

} // namespace yuzu::diagnostics

#endif // YUZU_DIAGNOSTICS_DIAGNOSTIC_H
