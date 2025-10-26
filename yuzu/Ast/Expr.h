#ifndef YUZU_AST_EXPR_H
#define YUZU_AST_EXPR_H

#include "yuzu/Ast/Ast.h"
#include "yuzu/Syntax/Syntax.h"

#include <variant>

namespace yuzu::ast {
class Expr;

class BinaryExpr final : public AstNode {
public:
  explicit BinaryExpr(syntax::SyntaxNode Node) : AstNode(std::move(Node)) {}

  BinaryExpr() = delete;
  BinaryExpr(const BinaryExpr &) = delete;
  BinaryExpr &operator=(const BinaryExpr &) = delete;
};

class ParenExpr final : public AstNode {
public:
  explicit ParenExpr(syntax::SyntaxNode Node) : AstNode(std::move(Node)) {}

  ParenExpr() = delete;
  ParenExpr(const ParenExpr &) = delete;
  ParenExpr &operator=(const ParenExpr &) = delete;
};

class LiteralExpr final : public AstNode {
public:
  explicit LiteralExpr(syntax::SyntaxNode Node) : AstNode(std::move(Node)) {}

  LiteralExpr() = delete;
  LiteralExpr(const LiteralExpr &) = delete;
  LiteralExpr &operator=(const LiteralExpr &) = delete;
};

class Expr : public std::variant<BinaryExpr, ParenExpr, LiteralExpr> {
  using std::variant<BinaryExpr, ParenExpr, LiteralExpr>::variant;

  Expr() = delete;

  template <typename Subtype>[[nodiscard]] bool is() const noexcept {
    return std::holds_alternative<Subtype>(*this);
  }

  template <typename Subtype>
  [[nodiscard]] std::optional<const Subtype *const> tryAs() const {
    if (const Subtype *const Value = std::get_if<Subtype>(this)) {
      return Value;
    }

    return std::nullopt;
  }
};
} // namespace yuzu::ast

#endif // YUZU_AST_EXPR_H
