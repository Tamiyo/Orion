#include "yuzu/Parser/Grammar/Grammar.h"

#include "yuzu/Parser/Grammar/Stmt.h"
#include "yuzu/Parser/Marker.h"
#include "yuzu/Parser/Parser.h"

#include <optional>

namespace yuzu::parser {

std::optional<CompletedMarker> parseRoot(Parser &p) {
  const auto m = p.start();

  // Loop over top-level statements, recovering past malformed ones rather
  // than bailing on first failure. `parseStmt` (via `parseLhs`) reports its
  // own diagnostic and bumps the offending token before returning nullopt,
  // so the parser is guaranteed to make progress between iterations and
  // the rest of the input still gets a chance to parse.
  while (!p.atEnd()) {
    auto _ = parseStmt(p);
  }

  return p.complete(m, ast::SyntaxKind::Root);
}

} // namespace yuzu::parser
