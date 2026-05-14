#include "yuzu/Hir/HirPrinter.h"

#include "yuzu/Util/ErrorHandling.h"

namespace yuzu::hir {

void HirPrinter::printNode(const HirNode *node, std::size_t indent) {
  switch (node->getKind()) {
  case HirKind::Root:
    printRoot(Root::cast(node), indent);
    return;
  case HirKind::ExprStmt:
    printExprStmt(ExprStmt::cast(node), indent);
    return;
  case HirKind::BinaryExpr:
    printBinaryExpr(BinaryExpr::cast(node), indent);
    return;
  case HirKind::LiteralExpr:
    printLiteralExpr(LiteralExpr::cast(node), indent);
    return;
  default:
    util::yuzu_unreachable();
  }
}

void HirPrinter::printRoot(const Root *root, std::size_t indent) {
  os.indent(indent * 2);
  os << "Root";
  for (const Stmt *stmt : root->getStmts()) {
    os << '\n';
    printNode(stmt, indent + 1);
  }
}

void HirPrinter::printExprStmt(const ExprStmt *stmt, std::size_t indent) {
  os.indent(indent * 2);
  os << "ExprStmt\n";
  printNode(stmt->getExpr(), indent + 1);
}

void HirPrinter::printBinaryExpr(const BinaryExpr *expr, std::size_t indent) {
  os.indent(indent * 2);
  os << "BinaryExpr op=" << asString(expr->getOp()) << '\n';
  printNode(expr->getLhs(), indent + 1);
  os << '\n';
  printNode(expr->getRhs(), indent + 1);
}

void HirPrinter::printLiteralExpr(const LiteralExpr *expr,
                                  std::size_t indent) {
  os.indent(indent * 2);
  os << "LiteralExpr value=" << expr->getValue();
}

} // namespace yuzu::hir
