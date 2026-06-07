#include "yuzu/Parser/Grammar/Type.h"

#include "yuzu/Lexer/TokenKind.h"
#include "yuzu/Parser/Grammar/Grammar.h"
#include "yuzu/Parser/Marker.h"
#include "yuzu/Parser/Parser.h"

#include <optional>

namespace yuzu::parser {
using namespace yuzu::ast;
using namespace yuzu::lexer;

namespace {
/// `NamedType := Identifier ( '[' Type (',' Type)* ']' )?`
///
/// A bare name (`string`, `Employee`) or a name applied to type
/// arguments (`Relation[Employee]`, `Aggregate[decimal, decimal]`). Each
/// argument recurses through `parseType`, so nesting falls out for free.
std::optional<CompletedMarker> parseNamedType(Parser &p) {
  const Marker m = p.start();
  const auto _ = parseIdent(p);

  if (p.at(TokenKind::LeftBracket)) {
    p.bump(); // '['
    parseType(p);
    while (p.at(TokenKind::Comma)) {
      p.bump(); // ','
      parseType(p);
    }
    p.expect(TokenKind::RightBracket);
  }

  return p.complete(m, SyntaxKind::NamedTypeAnnotation);
}

/// `RecordField := Identifier ':' Type` — one `name: type` member of a
/// record type.
std::optional<CompletedMarker> parseRecordField(Parser &p) {
  const Marker m = p.start();
  const auto _ = parseIdent(p);
  p.expect(TokenKind::Colon);
  parseType(p);
  return p.complete(m, SyntaxKind::RecordField);
}

/// Both record and function types open with `(`, so disambiguate on a
/// two-token lookahead: `'(' Identifier ':'` starts a record field;
/// anything else (including `()`) is a function type's parameter list.
///
///   RecordType   := '(' RecordField (',' RecordField)* ')'
///   FuncType     := FnTypeParams '->' Type
///   FnTypeParams := '(' ( Type (',' Type)* )? ')'
std::optional<CompletedMarker> parseParenType(Parser &p) {
  const bool isRecord = p.peekKind(1) == TokenKind::Identifier &&
                        p.peekKind(2) == TokenKind::Colon;

  const Marker m = p.start();

  if (isRecord) {
    p.expect(TokenKind::LeftParen);
    parseRecordField(p);
    while (p.at(TokenKind::Comma)) {
      p.bump(); // ','
      parseRecordField(p);
    }
    p.expect(TokenKind::RightParen);
    return p.complete(m, SyntaxKind::RecordType);
  }

  // Function type: the parenthesized parameter list is its own node so it
  // can't be confused with the result type, then the `->` arrow and result.
  const Marker params = p.start();
  p.expect(TokenKind::LeftParen);
  if (!p.at(TokenKind::RightParen)) {
    parseType(p);
    while (p.at(TokenKind::Comma)) {
      p.bump(); // ','
      parseType(p);
    }
  }
  p.expect(TokenKind::RightParen);
  const auto _ = p.complete(params, SyntaxKind::FuncTypeAnnotationParams);

  p.expect(TokenKind::Arrow);
  parseType(p);
  return p.complete(m, SyntaxKind::FuncTypeAnnotation);
}
} // namespace

std::optional<CompletedMarker> parseType(Parser &p) {
  if (p.peekKind() == TokenKind::LeftParen) {
    return parseParenType(p);
  }
  return parseNamedType(p);
}
} // namespace yuzu::parser
