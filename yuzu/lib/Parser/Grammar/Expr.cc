#include "yuzu/Parser/Grammar/Expr.h"

#include "yuzu/Ast/Ast.h"
#include "yuzu/Lexer/TokenKind.h"
#include "yuzu/Parser/Grammar/Grammar.h"
#include "yuzu/Parser/Marker.h"
#include "yuzu/Parser/Parser.h"

#include <bitset>
#include <cstdint>
#include <optional>
#include <utility>

namespace yuzu::parser {

using yuzu::ast::BinOp;
using yuzu::ast::SyntaxKind;
using yuzu::ast::UnaryOp;
using yuzu::lexer::TokenKind;

namespace {
constexpr std::bitset<1 << (8 * sizeof(lexer::TokenKind))> exprRecoverySet{};

std::pair<uint8_t, uint8_t> bindingPowerOf(UnaryOp op) {
  switch (op) {
  case UnaryOp::Neg:
  case UnaryOp::Pos:
    // Tighter than `*`/`/` but looser than `**`, so `-2 ** 2` is `-(2 ** 2)`.
    return std::make_pair(-1, 13);
  case UnaryOp::Not:
    // Looser than the comparison operators but tighter than `and`/`or`, so
    // `not a == b` is `not (a == b)` and `not a and b` is `(not a) and b`.
    return std::make_pair(-1, 5);
  }

  util::yuzu_unreachable();
}

std::pair<uint8_t, uint8_t> bindingPowerOf(BinOp op) {
  switch (op) {
  case BinOp::Or:
    return std::make_pair(1, 2);
  case BinOp::And:
    return std::make_pair(3, 4);
  case BinOp::Eq:
  case BinOp::Neq:
  case BinOp::In:
  case BinOp::NotIn:
  case BinOp::Lt:
  case BinOp::Lte:
  case BinOp::Gt:
  case BinOp::Gte:
    return std::make_pair(5, 6);
  case BinOp::ShiftLeft:
  case BinOp::ShiftRight:
    return std::make_pair(7, 8);
  case BinOp::Add:
  case BinOp::Sub:
    return std::make_pair(9, 10);
  case BinOp::Mul:
  case BinOp::Div:
    return std::make_pair(11, 12);
  case BinOp::Pow:
    // Right-associative (left power > right power), so `2 ** 3 ** 4` is
    // `2 ** (3 ** 4)`, and binds tighter than everything else.
    return std::make_pair(15, 14);
  }

  util::yuzu_unreachable();
}
}; // namespace

std::optional<BinOp> parseBinOp(Parser &p) {
  const auto kind = p.peekKind();
  if (!kind.has_value()) {
    return std::nullopt;
  }

  switch (kind.value()) {
  case TokenKind::Plus:
    return BinOp::Add;
  case TokenKind::Minus:
    return BinOp::Sub;
  case TokenKind::Star:
    return BinOp::Mul;
  case TokenKind::Pow:
    return BinOp::Pow;
  case TokenKind::Slash:
    return BinOp::Div;
  case TokenKind::EqEq:
    return BinOp::Eq;
  case TokenKind::Neq:
    return BinOp::Neq;
  case TokenKind::AndKw:
    return BinOp::And;
  case TokenKind::OrKw:
    return BinOp::Or;
  case TokenKind::Lt:
    return BinOp::Lt;
  case TokenKind::Lte:
    return BinOp::Lte;
  case TokenKind::Gt:
    return BinOp::Gt;
  case TokenKind::Gte:
    return BinOp::Gte;
  case TokenKind::ShiftLeft:
    return BinOp::ShiftLeft;
  case TokenKind::ShiftRight:
    return BinOp::ShiftRight;
  case TokenKind::InKw:
    return BinOp::In;
  case TokenKind::NotKw:
    // `not` is a binary operator only as the first half of `not in`; a bare
    // `not` in operator position is not an operator (prefix `not` is handled
    // by parseLhs).
    if (p.peekKind(1) == TokenKind::InKw) {
      return BinOp::NotIn;
    }
    return std::nullopt;
  default:
    return std::nullopt;
  }
}

std::optional<UnaryOp> parseUnaryOp(Parser &p) {
  const auto kind = p.peekKind();
  if (!kind.has_value()) {
    return std::nullopt;
  }

  switch (kind.value()) {
  case TokenKind::Plus:
    return UnaryOp::Pos;
  case TokenKind::Minus:
    return UnaryOp::Neg;
  case TokenKind::NotKw:
    return UnaryOp::Not;
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

/// `StructFieldInit := Identifier ':' Expr` — one `name: value` initializer.
std::optional<CompletedMarker> parseStructFieldInit(Parser &p) {
  const Marker m = p.start();
  const auto _ = parseIdent(p); // field name
  p.expect(TokenKind::Colon);
  parseExpr(p); // value
  return p.complete(m, SyntaxKind::StructFieldInit);
}

/// `StructExpr := Identifier '{' ( StructFieldInit (',' StructFieldInit)* ','?
/// )? '}'` — a struct literal like `Employee { id: 1, name: "Bob" }`.
std::optional<CompletedMarker> parseStructExpr(Parser &p) {
  const Marker m = p.start();
  const auto _ = parseIdent(p); // struct type name
  p.expect(TokenKind::LeftCurly);
  if (!p.at(TokenKind::RightCurly)) {
    parseStructFieldInit(p);
    while (p.at(TokenKind::Comma)) {
      p.bump(); // ','
      if (p.at(TokenKind::RightCurly)) {
        break; // trailing comma
      }
      parseStructFieldInit(p);
    }
  }
  p.expect(TokenKind::RightCurly);
  return p.complete(m, SyntaxKind::StructExpr);
}

std::optional<CompletedMarker> parseIdentExpr(Parser &p) {
  const Marker m = p.start();

  // The schema declares `IdentExpr` as `Child<Ident>:$name`, so the
  // tree must nest an `Ident` node inside the `IdentExpr` — not bury the
  // `Identifier` token directly. `parseIdent` builds that nested node.
  const auto _ = parseIdent(p);

  return p.complete(m, SyntaxKind::IdentExpr);
}

std::optional<CompletedMarker> parseParenExpr(Parser &p) {
  // Start the marker before the `(` so both parens nest inside the
  // `ParenExpr` node; otherwise they leak out as siblings of an enclosing
  // expression and break operand/operator lookup on it.
  const Marker m = p.start();
  p.expect(TokenKind::LeftParen); // Consume '('.

  const auto _ = parseExprBindingPower(p, 0);
  p.expect(TokenKind::RightParen);

  // TODO - Perhaps have a specific error for unclosed parenthesis?

  return p.complete(m, SyntaxKind::ParenExpr);
}

std::optional<CompletedMarker> parseUnaryExpr(Parser &p) {
  const auto op = parseUnaryOp(p);
  const auto rightBindingPower = bindingPowerOf(*op).second;

  const Marker m = p.start();
  p.bump(); // Consume the prefix operator.

  // The operand is parsed at the operator's right binding power.
  const auto _ = parseExprBindingPower(p, rightBindingPower);

  return p.complete(m, SyntaxKind::UnaryExpr);
}

/// `from <relation> [as] <alias>` — the pipe source. The alias is optional and
/// may be written with or without `as` (`from t e` or `from t as e`).
std::optional<CompletedMarker> parseFromExpr(Parser &p) {
  const Marker m = p.start();
  p.expect(TokenKind::FromKw);
  const auto _ = parseIdent(p); // relation

  if (p.at(TokenKind::AsKw)) {
    p.bump(); // 'as'
    [[maybe_unused]] const auto alias = parseIdent(p);
  } else if (p.at(TokenKind::Identifier)) {
    [[maybe_unused]] const auto alias = parseIdent(p); // bare alias
  }

  return p.complete(m, SyntaxKind::FromExpr);
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
    // `Name { ... }` is a struct literal; a bare `Name` is an identifier.
    if (p.peekKind(1) == TokenKind::LeftCurly) {
      return parseStructExpr(p);
    }
    return parseIdentExpr(p);

  case TokenKind::LeftParen:
    return parseParenExpr(p);

  case TokenKind::Plus:
  case TokenKind::Minus:
  case TokenKind::NotKw:
    return parseUnaryExpr(p);

  default:
    p.errorExpression(exprRecoverySet);
    return std::nullopt;
  }
}

/// `ArgList := '(' ( Expr (',' Expr)* )? ')'` — the parenthesized arguments
/// of a call, parsed into their own node.
std::optional<CompletedMarker> parseArgList(Parser &p) {
  const Marker m = p.start();
  p.expect(TokenKind::LeftParen);
  if (!p.at(TokenKind::RightParen)) {
    parseExprBindingPower(p, 0);
    while (p.at(TokenKind::Comma)) {
      p.bump(); // ','
      parseExprBindingPower(p, 0);
    }
  }
  p.expect(TokenKind::RightParen);
  return p.complete(m, SyntaxKind::ArgList);
}

/// `<expr> [as <alias>]` — one projected column of a `select` stage.
std::optional<CompletedMarker> parseSelectItem(Parser &p) {
  const Marker m = p.start();
  parseExprBindingPower(p, 0); // the row expression

  if (p.at(TokenKind::AsKw)) {
    p.bump(); // 'as'
    [[maybe_unused]] const auto alias = parseIdent(p);
  }

  return p.complete(m, SyntaxKind::SelectItem);
}

/// `select <item> (',' <item>)*` — the projection list of a `|> select` stage.
void parseSelectClause(Parser &p) {
  p.expect(TokenKind::SelectKw);
  parseSelectItem(p);
  while (p.at(TokenKind::Comma)) {
    p.bump(); // ','
    parseSelectItem(p);
  }
}

/// `where <predicate>` — the row filter of a `|> where` stage.
void parseWhereClause(Parser &p) {
  p.expect(TokenKind::WhereKw);
  parseExprBindingPower(p, 0); // the predicate
}

/// `distinct` — the dedupe of a `|> distinct` stage (no operands).
void parseDistinctClause(Parser &p) { p.expect(TokenKind::DistinctKw); }

/// `drop <column> (',' <column>)*` — the columns a `|> drop` stage removes.
void parseDropClause(Parser &p) {
  p.expect(TokenKind::DropKw);
  [[maybe_unused]] const auto first = parseIdent(p);
  while (p.at(TokenKind::Comma)) {
    p.bump(); // ','
    [[maybe_unused]] const auto next = parseIdent(p);
  }
}

/// `<from> as <to>` — one rename of a `|> rename` stage.
void parseRenameItem(Parser &p) {
  const Marker m = p.start();
  [[maybe_unused]] const auto from = parseIdent(p);
  p.expect(TokenKind::AsKw);
  [[maybe_unused]] const auto to = parseIdent(p);
  [[maybe_unused]] const auto item = p.complete(m, SyntaxKind::RenameItem);
}

/// `rename <item> (',' <item>)*` — the renames of a `|> rename` stage.
void parseRenameClause(Parser &p) {
  p.expect(TokenKind::RenameKw);
  parseRenameItem(p);
  while (p.at(TokenKind::Comma)) {
    p.bump(); // ','
    parseRenameItem(p);
  }
}

/// `extend <item> (',' <item>)*` — the appended columns of a `|> extend` stage.
/// Each item is a `select`-style `<expr> [as <alias>]`.
void parseExtendClause(Parser &p) {
  p.expect(TokenKind::ExtendKw);
  parseSelectItem(p);
  while (p.at(TokenKind::Comma)) {
    p.bump(); // ','
    parseSelectItem(p);
  }
}

/// A pipe query: `from <rel> [as] <alias> ( |> <stage> )*`. A query is *not* a
/// general subexpression — it's parsed only in value positions (a standalone
/// statement, or a `let`/assignment RHS) so a relation can't be wedged into a
/// scalar context like `(from t |> select x) + 1`. Each `|>` stage wraps the
/// running relation, so stages nest left-to-right.
std::optional<CompletedMarker> parseQuery(Parser &p) {
  std::optional<CompletedMarker> query = parseFromExpr(p);
  if (!query.has_value()) {
    return std::nullopt;
  }

  while (p.at(TokenKind::Pipe)) {
    const auto [marker, _] = p.precede(*query);
    p.bump(); // '|>'
    if (p.at(TokenKind::WhereKw)) {
      parseWhereClause(p);
      query.emplace(p.complete(marker, SyntaxKind::WhereExpr));
    } else if (p.at(TokenKind::DistinctKw)) {
      parseDistinctClause(p);
      query.emplace(p.complete(marker, SyntaxKind::DistinctExpr));
    } else if (p.at(TokenKind::DropKw)) {
      parseDropClause(p);
      query.emplace(p.complete(marker, SyntaxKind::DropExpr));
    } else if (p.at(TokenKind::RenameKw)) {
      parseRenameClause(p);
      query.emplace(p.complete(marker, SyntaxKind::RenameExpr));
    } else if (p.at(TokenKind::ExtendKw)) {
      parseExtendClause(p);
      query.emplace(p.complete(marker, SyntaxKind::ExtendExpr));
    } else {
      // `select` is the default clause; `parseSelectClause` reports a
      // diagnostic if the keyword is missing.
      parseSelectClause(p);
      query.emplace(p.complete(marker, SyntaxKind::SelectExpr));
    }
  }

  return query;
}

std::optional<CompletedMarker>
parseExprBindingPower(Parser &p, const size_t minimumBindingPower) {
  std::optional<CompletedMarker> parsedLhs = parseLhs(p);
  if (!parsedLhs.has_value()) {
    return std::nullopt;
  }

  while (true) {
    // Postfix call: `lhs(...)`. Binds tighter than any binary operator, so
    // it attaches to the immediate LHS before the operator loop runs. The
    // completed LHS becomes the callee, wrapping into a `CallExpr`.
    if (p.at(TokenKind::LeftParen)) {
      const auto [marker, _] = p.precede(*parsedLhs);
      parseArgList(p);
      parsedLhs.emplace(p.complete(marker, SyntaxKind::CallExpr));
      continue;
    }

    // Postfix field access: `lhs.field`. Binds as tightly as a call, attaching
    // to the immediate LHS and wrapping into a `FieldAccessExpr`.
    if (p.at(TokenKind::Dot)) {
      const auto [marker, _] = p.precede(*parsedLhs);
      p.bump(); // '.'
      [[maybe_unused]] const auto field = parseIdent(p);
      parsedLhs.emplace(p.complete(marker, SyntaxKind::FieldAccessExpr));
      continue;
    }

    // Stop if we are not at a binary operator.
    const auto op = parseBinOp(p);
    if (!op.has_value()) {
      break;
    }

    // Stop if the operator binds less tightly than the caller requires.
    const auto [leftBindingPower, rightBindingPower] = bindingPowerOf(*op);
    if (leftBindingPower < minimumBindingPower) {
      break;
    }

    // Consume the operator. `not in` spans two tokens, both of which become
    // children of the BinaryExpr.
    p.bump();
    if (*op == BinOp::NotIn) {
      p.bump();
    }

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
