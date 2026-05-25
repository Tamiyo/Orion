#include "yuzu/Parser/Grammar/Expr.h"

#include "yuzu/Ast/Ast.h"
#include "yuzu/Lexer/TokenKind.h"
#include "yuzu/Parser/Marker.h"
#include "yuzu/Parser/Parser.h"

#include <bitset>
#include <cstdint>
#include <optional>
#include <utility>

namespace yuzu::parser {

using yuzu::ast::SyntaxKind;
using yuzu::lexer::TokenKind;

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
  case TokenKind::Plus:
    return BinaryOp::Add;
  case TokenKind::Minus:
    return BinaryOp::Sub;
  case TokenKind::Star:
    return BinaryOp::Mul;
  case TokenKind::Slash:
    return BinaryOp::Div;
  default:
    return std::nullopt;
  }
}

std::optional<CompletedMarker> parseLiteralExpr(Parser &p) {
  const auto kind = p.peekKind();

  // Map the lexer's literal-token kind to the corresponding AST node.
  SyntaxKind astKind;
  switch (*kind) {
  case TokenKind::IntegerLiteral:
  case TokenKind::HexLiteral:
  case TokenKind::BinaryLiteral:
    astKind = SyntaxKind::IntLit;
    break;
  case TokenKind::FloatLiteral:
    astKind = SyntaxKind::FloatLit;
    break;
  case TokenKind::StringLiteral:
  case TokenKind::RawStringLiteral:
    astKind = SyntaxKind::StringLit;
    break;
  case TokenKind::BooleanLiteral:
    // TODO: add `BoolLit : Node<Literal>` to the AST schema and route
    // boolean tokens to it. Until then they piggyback on `IntLit` so the
    // tree is well-formed.
    astKind = SyntaxKind::BoolLit;
    break;
  default:
    util::yuzu_unreachable();
  }

  const Marker m = p.start();
  p.bump(); // Consume the literal token.
  return p.complete(m, astKind);
}

std::optional<CompletedMarker> parseIdentExpr(Parser &p) {
  const Marker m = p.start();

  // The schema declares `IdentExpr` as `Child<Ident>:$name`, so the
  // tree must nest an `Ident` node inside the `IdentExpr` — not
  // bury the `Identifier` token directly. The AST accessor digs out
  // the child `Ident` via the schema's child-iteration path.
  const Marker inner = p.start();
  p.expect(TokenKind::Identifier);
  const auto _ = p.complete(inner, SyntaxKind::Ident);

  return p.complete(m, SyntaxKind::IdentExpr);
}

std::optional<CompletedMarker> parseParenExpr(Parser &p) {
  p.expect(TokenKind::LeftParen); // Consume '('.

  const auto expr = parseExprBindingPower(p, 0);
  p.expect(TokenKind::RightParen);

  // TODO - Perhaps have a specific error for unclosed parenthesis?

  return expr;
}

std::optional<CompletedMarker> parseLhs(Parser &p) {
  const auto kind = p.peekKind();
  if (!kind.has_value()) {
    // End of input where an expression was required (e.g. `1 +` then EOF).
    // Emit the same `expected expression, found ...` diagnostic the
    // default arm uses so callers see a single recoverable error.
    p.errorExpression(exprRecoverySet);
    return std::nullopt;
  }

  switch (*kind) {
  case TokenKind::BooleanLiteral:
  case TokenKind::IntegerLiteral:
  case TokenKind::FloatLiteral:
  case TokenKind::HexLiteral:
  case TokenKind::BinaryLiteral:
  case TokenKind::StringLiteral:
  case TokenKind::RawStringLiteral:
    return parseLiteralExpr(p);

  case TokenKind::Identifier:
    return parseIdentExpr(p);

  case TokenKind::LeftParen:
    return parseParenExpr(p);

  default:
    // Semantic version: emits `expected expression, found `<text>`` instead
    // of enumerating the LHS kinds. Same recovery shape as `error`.
    p.errorExpression(exprRecoverySet);
    return std::nullopt;
  }
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
    parsedLhs.emplace(p.complete(marker, SyntaxKind::BinaryExpr));
    if (!parsedRhs.has_value()) {
      break;
    }
  }

  return parsedLhs;
}
} // namespace yuzu::parser
