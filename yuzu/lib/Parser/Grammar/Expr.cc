#include "yuzu/Parser/Grammar/Expr.h"

#include "yuzu/Lexer/TokenKind.h"
#include "yuzu/Parser/Marker.h"
#include "yuzu/Parser/Parser.h"

#include <bitset>
#include <cassert>
#include <optional>

namespace yuzu::parser {
namespace {
constexpr std::bitset<1 << (8 * sizeof(lexer::TokenKind))> exprRecoverySet{};
};

std::optional<CompletedMarker> parseLiteral(Parser &parser) {
  assert(parser.peekKind() == lexer::TokenKind::Number &&
         "Literals must be numbers.");

  const Marker marker = parser.start();
  parser.bump();
  return parser.complete(marker, ast::SyntaxKind::LiteralExpr);
}

std::optional<CompletedMarker> parseIdent(Parser &parser) {
  assert(parser.peekKind() == lexer::TokenKind::Ident &&
         "Variable references must be identifiers.");

  const Marker marker = parser.start();
  parser.bump();
  return parser.complete(marker, ast::SyntaxKind::Ident);
}

std::optional<CompletedMarker> lhs(Parser &parser) {
  const std::optional<lexer::TokenKind> kind = parser.peekKind();
  if (!kind.has_value()) {
    parser.error(exprRecoverySet);
    return std::nullopt;
  }

  switch (kind.value()) {
  case lexer::TokenKind::Number:
    return parseLiteral(parser);
  case lexer::TokenKind::Ident:
    return parseIdent(parser);
  default:
    parser.error(exprRecoverySet);
    return std::nullopt;
  }
}

std::optional<CompletedMarker>
parseExprBindingPower(Parser &parser, const size_t minimumBindingPower) {
  const std::optional<CompletedMarker> completedMarker = lhs(parser);
  assert(minimumBindingPower > 0 &&
         "Pratt parser requires positive minimum binding power.");

  // TODO(tamiyo) Continue with Pratt Parsing implementation.

  return completedMarker;
}
} // namespace yuzu::parser
