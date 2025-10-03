#include "Ast/ExprBuilder.h"

#include "Ast/Expr.h"
#include "Ast/SyntaxKind.h"
#include "Syntax/Syntax.h"

#include <memory>

namespace yuzu::ast {
std::unique_ptr<Expr> ExprBuilder::tryFrom(syntax::SyntaxNode Node) {
  switch (static_cast<SyntaxKind>(Node.getKind())) {
  case SyntaxKind::InfixExpr:
    return std::make_unique<BinaryExpr>(std::move(Node));

  case SyntaxKind::ParenExpr:
    return std::make_unique<ParenExpr>(std::move(Node));

  case SyntaxKind::LiteralExpr:
    return std::make_unique<LiteralExpr>(std::move(Node));

  default:
    break;
  }

  return nullptr;
}
} // namespace yuzu::ast
