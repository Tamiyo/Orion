#ifndef LANG_AST_EXPR_H_
#define LANG_AST_EXPR_H_

#include <optional>
#include <variant>

#include "lang/ast/syntax.h"
#include "lang/parser/syntax_kind.h"

namespace yuzu::lang {
class Expr;


// TODO(tamiyo) Implement this.
class BinaryExpr {
 public:
  explicit BinaryExpr(SyntaxNode syntax) : syntax_(syntax) {}
  BinaryExpr() = delete;

  std::optional<Expr> Lhs() const noexcept;
  std::optional<Expr> Rhs() const noexcept;
  std::optional<SyntaxToken> Op() const noexcept;

 private:
  const SyntaxNode syntax_;
};

// TODO(tamiyo) Implement this.
class Expr {
 public:
  using Variant = std::variant<BinaryExpr>;

  static std::optional<Expr> Cast(const SyntaxNode& node);

  explicit Expr(Variant variant) : variant_(variant) {}
  Expr() = delete;

 private:
  const Variant variant_;
};
}  // namespace yuzu::lang

#endif  // LANG_AST_EXPR_H_
