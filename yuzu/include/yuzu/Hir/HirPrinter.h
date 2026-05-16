#ifndef YUZU_HIR_HIR_PRINTER_H
#define YUZU_HIR_HIR_PRINTER_H

#include "yuzu/Hir/Hir.h"

#include "llvm/Support/raw_ostream.h"

#include <cstddef>
#include <string>

namespace yuzu::hir {

/// Pretty-prints an HIR tree as an indented kind tree. One line per node,
/// two-space indent per depth, no trailing newline:
///
///   Root
///     ExprStmt
///       BinaryExpr op=Add
///         LiteralExpr
///         LiteralExpr
class [[nodiscard]] HirPrinter final {
public:
  explicit HirPrinter(llvm::raw_ostream &os) : os(os) {}

  HirPrinter() = delete;

  void print(const HirNode *node) { printNode(node, 0); }

  static std::string printToString(const HirNode *node) {
    std::string out;
    llvm::raw_string_ostream stream(out);
    HirPrinter(stream).print(node);
    return out;
  }

private:
  void printNode(const HirNode *node, std::size_t indent);
  void printRoot(const Root *root, std::size_t indent);
  void printExprStmt(const ExprStmt *stmt, std::size_t indent);
  void printBinaryExpr(const BinaryExpr *expr, std::size_t indent);
  void printIntLit(const IntLit *expr,
                               std::size_t indent);
  void printFloatLit(const FloatLit *expr,
                             std::size_t indent);
  void printStringLit(const StringLit *expr,
                              std::size_t indent);

  llvm::raw_ostream &os;
};

} // namespace yuzu::hir

#endif // YUZU_HIR_HIR_PRINTER_H
