#include "yuzu/Parser/Grammar/Stmt.h"

#include "yuzu/Lexer/TokenKind.h"
#include "yuzu/Parser/Grammar/Expr.h"
#include "yuzu/Parser/Grammar/Grammar.h"
#include "yuzu/Parser/Grammar/Type.h"
#include "yuzu/Parser/Marker.h"
#include "yuzu/Parser/Parser.h"

#include <optional>

namespace yuzu::parser {
using namespace yuzu::ast;
using namespace yuzu::lexer;

namespace {
inline std::optional<CompletedMarker> parseLetStmt(Parser &p) {
  const Marker m = p.start();
  p.expect(TokenKind::LetKw);

  const auto _ = parseIdent(p);

  // Optional type annotation: `let x: int = ...`.
  if (p.at(TokenKind::Colon)) {
    p.bump(); // ':'
    parseType(p);
  }

  p.expect(TokenKind::Eq);
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
