#ifndef AST_EXPRBUILDER_H
#define AST_EXPRBUILDER_H

#include "Syntax/Syntax.h"

#include <memory>
#include <optional>

namespace yuzu::ast {
class Expr;

class ExprBuilder {
public:
  [[nodiscard]] static std::unique_ptr<Expr>
  tryFrom(syntax::SyntaxNode Node);
};
} // namespace yuzu::ast

#endif // AST_EXPRBUILDER_H
