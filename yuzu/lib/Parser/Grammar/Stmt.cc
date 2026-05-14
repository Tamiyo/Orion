#include "yuzu/Parser/Grammar/Stmt.h"

#include "yuzu/Parser/Grammar/Expr.h"
#include "yuzu/Parser/Marker.h"
#include "yuzu/Parser/Parser.h"

#include <optional>

namespace yuzu::parser {
std::optional<CompletedMarker> parseStmt(Parser &p) {
  const Marker m = p.start();
  auto _ = parseExpr(p);
  return p.complete(m, ast::SyntaxKind::ExprStmt);
}
} // namespace yuzu::parser
