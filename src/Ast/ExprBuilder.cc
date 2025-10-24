#include "Ast/ExprBuilder.h"

#include "Ast/Expr.h"
#include "Ast/SyntaxKind.h"
#include "Syntax/Syntax.h"

#include <memory>
#include <utility>

namespace yuzu::ast {
std::unique_ptr<Expr> ExprBuilder::tryFrom(const syntax::SyntaxNode &Node) {
  switch (static_cast<SyntaxKind>(Node.getKind())) {
  case SyntaxKind::InfixExpr:
    return std::make_unique<Expr>(std::in_place_type<BinaryExpr>, Node);

  case SyntaxKind::ParenExpr:
    return std::make_unique<Expr>(std::in_place_type<ParenExpr>, Node);

  case SyntaxKind::LiteralExpr:
    return std::make_unique<Expr>(std::in_place_type<LiteralExpr>, Node);

  default:
    break;
  }

  return nullptr;
}
} // namespace yuzu::ast
