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
  case HirKind::IntLit:
    printIntLit(IntLit::cast(node), indent);
    return;
  case HirKind::FloatLit:
    printFloatLit(FloatLit::cast(node), indent);
    return;
  case HirKind::StringLit:
    printStringLit(StringLit::cast(node), indent);
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

void HirPrinter::printIntLit(const IntLit *expr,
                                         std::size_t indent) {
  os.indent(indent * 2);
  os << "IntLit value=" << expr->getValue();
}

void HirPrinter::printFloatLit(const FloatLit *expr,
                                       std::size_t indent) {
  os.indent(indent * 2);
  os << "FloatLit value=" << expr->getValue();
}

void HirPrinter::printStringLit(const StringLit *expr,
                                        std::size_t indent) {
  os.indent(indent * 2);
  os << "StringLit isRaw=" << (expr->getIsRaw() ? "true" : "false");
}

} // namespace yuzu::hir
