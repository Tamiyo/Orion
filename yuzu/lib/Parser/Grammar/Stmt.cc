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
std::optional<CompletedMarker> parseBlockStmt(Parser &p) {
  const Marker m = p.start();

  p.expect(TokenKind::LeftCurly);

  // `atEnd` guards against an unclosed `{` — without it the loop would spin
  // once input runs out (`parseStmt` can't make progress past EOF).
  while (!p.at(TokenKind::RightCurly) && !p.atEnd()) {
    parseStmt(p);
  }

  p.expect(TokenKind::RightCurly);

  return p.complete(m, SyntaxKind::BlockStmt);
}

/// `TypeParam := Identifier` — one generic parameter name in `[ ]`.
/// Bounds (`T: Comparable`) are a later addition.
std::optional<CompletedMarker> parseTypeParam(Parser &p) {
  const Marker m = p.start();
  const auto _ = parseIdent(p);
  return p.complete(m, SyntaxKind::TypeParam);
}

/// `TraitRef := Identifier` — a trait named in a bound.
std::optional<CompletedMarker> parseTraitRef(Parser &p) {
  const Marker m = p.start();
  const auto _ = parseIdent(p);
  return p.complete(m, SyntaxKind::TraitRef);
}

/// `TypeBound := Identifier ':' TraitRef ('+' TraitRef)*` — one `where`
/// clause, e.g. `T: Add + Eq`.
std::optional<CompletedMarker> parseTypeBound(Parser &p) {
  const Marker m = p.start();
  const auto _ = parseIdent(p);
  p.expect(TokenKind::Colon);
  parseTraitRef(p);
  while (p.at(TokenKind::Plus)) {
    p.bump(); // '+'
    parseTraitRef(p);
  }
  return p.complete(m, SyntaxKind::TypeBound);
}

/// `Param := Identifier ':' Type` — one `name: type` function parameter.
std::optional<CompletedMarker> parseParam(Parser &p) {
  const Marker m = p.start();
  const auto _ = parseIdent(p);
  p.expect(TokenKind::Colon);
  parseType(p);
  return p.complete(m, SyntaxKind::Param);
}

/// `FuncStmt := 'fn' Identifier ( '[' TypeParam (',' TypeParam)* ']' )?
///           '(' ( Param (',' Param)* )? ')' ( '->' Type )? BlockStmt`
std::optional<CompletedMarker> parseFuncStmt(Parser &p) {
  const Marker m = p.start();

  p.expect(TokenKind::FnKw);
  const auto _ = parseIdent(p);

  // Optional generic type parameters: `[T, U]`.
  if (p.at(TokenKind::LeftBracket)) {
    p.bump(); // '['
    if (!p.at(TokenKind::RightBracket)) {
      parseTypeParam(p);
      while (p.at(TokenKind::Comma)) {
        p.bump(); // ','
        parseTypeParam(p);
      }
    }
    p.expect(TokenKind::RightBracket);
  }

  // Parameter list: `(x: int, y: str)`.
  p.expect(TokenKind::LeftParen);
  if (!p.at(TokenKind::RightParen)) {
    parseParam(p);
    while (p.at(TokenKind::Comma)) {
      p.bump(); // ','
      parseParam(p);
    }
  }
  p.expect(TokenKind::RightParen);

  // Optional return type: `-> int`.
  if (p.at(TokenKind::Arrow)) {
    p.bump(); // '->'
    parseType(p);
  }

  // Optional `where` clause: `where T: Add, U: Eq`.
  if (p.at(TokenKind::WhereKw)) {
    p.bump(); // 'where'
    parseTypeBound(p);
    while (p.at(TokenKind::Comma)) {
      p.bump(); // ','
      parseTypeBound(p);
    }
  }

  parseBlockStmt(p);

  return p.complete(m, SyntaxKind::FuncStmt);
}

/// `ReturnStmt := 'return' Expr?`
std::optional<CompletedMarker> parseReturnStmt(Parser &p) {
  const Marker m = p.start();
  p.expect(TokenKind::ReturnKw);

  // `return` may stand alone (e.g. before `}`); only parse an operand when
  // one is actually present.
  if (!p.at(TokenKind::RightCurly)) {
    parseExpr(p);
  }

  return p.complete(m, SyntaxKind::ReturnStmt);
}

std::optional<CompletedMarker> parseLetStmt(Parser &p) {
  const Marker m = p.start();
  p.expect(TokenKind::LetKw);

  // Optional `mut`: `let mut x = ...` binds a mutable variable.
  if (p.at(TokenKind::MutKw)) {
    p.bump(); // 'mut'
  }

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

/// A statement that starts with an expression: either a bare expression
/// (`f(x)`) or an assignment (`x = 5`) when an `=` follows it.
std::optional<CompletedMarker> parseExprStmt(Parser &p) {
  const Marker m = p.start();
  auto _ = parseExpr(p);

  if (p.at(TokenKind::Eq)) {
    p.bump(); // '='
    parseExpr(p);
    return p.complete(m, SyntaxKind::AssignStmt);
  }

  return p.complete(m, SyntaxKind::ExprStmt);
}
} // namespace

std::optional<CompletedMarker> parseStmt(Parser &p) {
  if (p.at(TokenKind::FnKw)) {
    return parseFuncStmt(p);
  }

  if (p.at(TokenKind::LetKw)) {
    return parseLetStmt(p);
  }

  if (p.at(TokenKind::ReturnKw)) {
    return parseReturnStmt(p);
  }

  return parseExprStmt(p);
}
} // namespace yuzu::parser
