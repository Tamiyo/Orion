#include "yuzu/Parser/Grammar/Expr.h"

#include "yuzu/Ast/Ast.h"
#include "yuzu/Lexer/TokenKind.h"
#include "yuzu/Parser/Marker.h"
#include "yuzu/Parser/Parser.h"

#include <bitset>
#include <cassert>
#include <cstdint>
#include <optional>
#include <utility>

namespace yuzu::parser {
namespace {
constexpr std::bitset<1 << (8 * sizeof(lexer::TokenKind))> exprRecoverySet{};

enum class BinaryOp : uint8_t { Add, Sub, Mul, Div };

std::pair<uint8_t, uint8_t> bindingPowerOf(BinaryOp op) {
  switch (op) {
  case BinaryOp::Add:
  case BinaryOp::Sub:
    return std::make_pair(1, 2);
  case BinaryOp::Mul:
  case BinaryOp::Div:
    return std::make_pair(3, 4);
  }
}
}; // namespace

std::optional<BinaryOp> parseBinaryOp(Parser &p) {
  const auto kind = p.peekKind();
  if (!kind.has_value()) {
    return std::nullopt;
  }

  switch (kind.value()) {
  case lexer::TokenKind::Plus:
    return BinaryOp::Add;
  case lexer::TokenKind::Minus:
    return BinaryOp::Sub;
  case lexer::TokenKind::Star:
    return BinaryOp::Mul;
  case lexer::TokenKind::Slash:
    return BinaryOp::Div;
  default:
    return std::nullopt;
  }
}

std::optional<CompletedMarker> parseLiteralExpr(Parser &p) {
  assert(p.peekKind() == lexer::TokenKind::Number &&
         "Literals must be numbers.");

  const Marker m = p.start();
  p.bump(); // Consume literal.
  return p.complete(m, ast::SyntaxKind::LiteralExpr);
}

std::optional<CompletedMarker> parseIdentExpr(Parser &p) {
  assert(p.peekKind() == lexer::TokenKind::Ident &&
         "Variable references must be identifiers.");

  const Marker m = p.start();

  p.bump(); // Consume identifier.
  return p.complete(m, ast::SyntaxKind::Ident);
}

std::optional<CompletedMarker> parseParenExpr(Parser &p) {
  assert(p.peekKind() == lexer::TokenKind::LeftParen &&
         "Expected a LeftParen.");

  p.bump(); // Consume '('.

  const auto expr = parseExprBindingPower(p, 0);
  p.expect(lexer::TokenKind::RightParen);

  // TODO - Perhaps have a specific error for unclosed parenthesis?

  return expr;
}

std::optional<CompletedMarker> parseLhs(Parser &p) {
  if (p.at(lexer::TokenKind::Number)) {
    return parseLiteralExpr(p);
  }

  if (p.at(lexer::TokenKind::Ident)) {
    return parseIdentExpr(p);
  }

  if (p.at(lexer::TokenKind::LeftParen)) {
    return parseParenExpr(p);
  }

  // Semantic version: emits `expected expression, found `<text>`` instead
  // of enumerating the LHS kinds. Same recovery shape as `error`.
  p.errorExpression(exprRecoverySet);
  return std::nullopt;
}

std::optional<CompletedMarker>
parseExprBindingPower(Parser &p, const size_t minimumBindingPower) {
  std::optional<CompletedMarker> parsedLhs = parseLhs(p);
  if (!parsedLhs.has_value()) {
    return std::nullopt;
  }

  while (true) {
    // Stop if we are not at a binary operator.
    const auto op = parseBinaryOp(p);
    if (!op.has_value()) {
      break;
    }

    // Stop if the operator binds less tightly than the caller requires.
    const auto [leftBindingPower, rightBindingPower] = bindingPowerOf(*op);
    if (leftBindingPower < minimumBindingPower) {
      break;
    }

    // Consume the operator.
    p.bump();

    // Parse the Rhs of the operation, if any.
    const auto [marker, _] = p.precede(*parsedLhs);
    const auto parsedRhs = parseExprBindingPower(p, rightBindingPower);

    // Wrap the LHS, operator, and (possibly missing) RHS in a BinaryExpr
    // before bailing — the recursive call has already reported the error.
    parsedLhs.emplace(p.complete(marker, ast::SyntaxKind::BinaryExpr));
    if (!parsedRhs.has_value()) {
      break;
    }
  }

  return parsedLhs;
}
} // namespace yuzu::parser
