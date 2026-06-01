#include "yuzu/Hir/Types/TypeConcretizer.h"

#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/HirContext.h"
#include "yuzu/Hir/Types/Type.h"

#include <llvm/Support/FormatVariadic.h>

namespace yuzu::hir {
namespace {

/// Range-check a literal that *directly* supplies a value (so the literal
/// is the whole value, not an operand of a larger expression). Literals
/// inside arithmetic (`128 - 20`) are deliberately skipped — judging those
/// needs constant folding, which is a later phase; checking them per-literal
/// gives both false positives (`128`) and false negatives (`126 + 127`).
void checkLiteralRange(const Expr *e, HirContext &ctx) {
  const Type *type = ctx.getTypeContext().typeOf(e);
  if (!type) {
    return;
  }

  if (const auto *lit = IntLit::cast(e);
      lit && type->isInt() && !type->canRepresent(lit->getValue())) {
    ctx.error(lit,
              llvm::formatv("integer literal {0} is out of range for `{1}`",
                            lit->getValue(), asString(type->getKind()))
                  .str())
        .emit();
  } else if (const auto *flit = FloatLit::cast(e);
             flit && type->isFloat() && !type->canRepresent(flit->getValue())) {
    ctx.error(flit, llvm::formatv("float literal {0} is out of range for `{1}`",
                                  flit->getValue(), asString(type->getKind()))
                        .str())
        .emit();
  }
}

} // namespace

void TypeConcretizer::visit(const HirNode *node) {
  // Base `visit` walks children first, so types resolve bottom-up.
  HirVisitor::visit(node);
  ctx.getTypeContext().concretize(node);

  // Only a literal that directly initializes a binding is range-checked for
  // now; expression initializers wait for constant folding.
  if (const auto *let = LetStmt::cast(node)) {
    checkLiteralRange(let->getExpr(), ctx);
  }
}

} // namespace yuzu::hir
