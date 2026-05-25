#include "yuzu/Parser/Grammar/Stmt.h"

#include "yuzu/Lexer/TokenKind.h"
#include "yuzu/Parser/Grammar/Expr.h"
#include "yuzu/Parser/Marker.h"
#include "yuzu/Parser/Parser.h"

#include <optional>

namespace yuzu::parser {
using namespace yuzu::ast;
using namespace yuzu::lexer;

namespace {
/// Parse an identifier as a *binding* — produces a `SyntaxKind::Ident`
/// marker, not `IdentExpr`. The distinction matches the schema split:
/// `Ident` is a name declaration (no type), `IdentExpr` is a use
/// (typed by name resolution). `let x = ...` wants the former.
inline std::optional<CompletedMarker> parseIdent(Parser &p) {
  const Marker m = p.start();
  p.expect(TokenKind::Identifier);
  return p.complete(m, SyntaxKind::Ident);
}

inline std::optional<CompletedMarker> parseLetStmt(Parser &p) {
  const Marker m = p.start();
  p.expect(TokenKind::LetKw);

  const auto _ = parseIdent(p);

  p.expect(TokenKind::Equals);
  parseExpr(p);
  return p.complete(m, SyntaxKind::LetStmt);
}

inline std::optional<CompletedMarker> parseExprStmt(Parser &p) {
  const Marker m = p.start();
  auto _ = parseExpr(p);
  return p.complete(m, SyntaxKind::ExprStmt);
}
} // namespace

std::optional<CompletedMarker> parseStmt(Parser &p) {
  if (p.at(TokenKind::LetKw)) {
    return parseLetStmt(p);
  }

  return parseExprStmt(p);
}
} // namespace yuzu::parser
