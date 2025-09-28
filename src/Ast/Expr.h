#ifndef AST_EXPR_H
#define AST_EXPR_H

#include "Syntax/Syntax.h"

#include <memory>
#include <optional>

namespace yuzu::ast {
class Expr {
  virtual std::optional<Expr> cast(const syntax::SyntaxNode &Node);
};

class BinaryExpr : public Expr {
public:
  std::optional<Expr> getLhs();
  std::optional<Expr> getRhs();

private:
  const syntax::SyntaxNode Node_;
};

class UnaryExpr : public Expr {
public:
  std::optional<Expr> getValue();

private:
  const syntax::SyntaxNode Node_;
};
} // namespace yuzu::ast

#endif // AST_EXPR_H
