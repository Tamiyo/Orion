#include "yuzu/Diagnostics/DiagnosticBuilder.h"

#include "yuzu/Diagnostics/DiagnosticsEngine.h"

#include <utility>

namespace yuzu::diagnostics {

void DiagnosticBuilder::emit() {
  // Defined out-of-line so the header doesn't need to know
  // `DiagnosticsEngine`'s full layout — the builder is constructed by
  // the diagnostics but it's the diagnostics that knows how to store the
  // resulting diagnostic.
  diagnostics->push(std::move(diag));
}

} // namespace yuzu::diagnostics
