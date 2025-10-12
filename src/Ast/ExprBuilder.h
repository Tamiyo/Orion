#ifndef AST_EXPR_BUILDER_H
#define AST_EXPR_BUILDER_H

#include "Syntax/Syntax.h"

#include <memory>

namespace yuzu::ast {
class Expr;

class ExprBuilder {
public:
  [[nodiscard]] static std::unique_ptr<Expr>
  tryFrom(const syntax::SyntaxNode &Node);
};
} // namespace yuzu::ast

#endif // AST_EXPR_BUILDER_H
