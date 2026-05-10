#include "yuzu/Parser/Grammar/Grammar.h"

#include "yuzu/Parser/Grammar/Stmt.h"
#include "yuzu/Parser/Marker.h"
#include "yuzu/Parser/Parser.h"

#include <optional>

namespace yuzu::parser {

std::optional<CompletedMarker> parseRoot(Parser &p) {
  const auto m = p.start();

  while (!p.atEnd()) {
    const auto stmt = parseStmt(p);
    if (!stmt.has_value()) {
      break;
    }
  }

  return p.complete(m, ast::SyntaxKind::Root);
}

} // namespace yuzu::parser
