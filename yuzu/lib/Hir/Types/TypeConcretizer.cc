#include "yuzu/Hir/Types/TypeConcretizer.h"

#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/HirContext.h"
#include "yuzu/Hir/Types/Type.h"

#include <llvm/Support/FormatVariadic.h>

namespace yuzu::hir {
namespace {

/// Range-check a literal that *directly* supplies a value (not an operand of
/// a larger expression — those wait for constant folding).
void checkLiteralRange(const Expr *e, HirContext &ctx) {
  const Type *type = ctx.getTypeContext().typeOf(e);
  if (!type) {
    return;
  }

  if (const auto *i = IntLit::cast(e);
      i && type->isInt() && !type->canRepresent(i->getValue())) {
    ctx.error(i, llvm::formatv("integer literal {0} is out of range for `{1}`",
                               i->getValue(), asString(type->getKind()))
                     .str())
        .emit();
  } else if (const auto *f = FloatLit::cast(e);
             f && type->isFloat() && !type->canRepresent(f->getValue())) {
    ctx.error(f, llvm::formatv("float literal {0} is out of range for `{1}`",
                               f->getValue(), asString(type->getKind()))
                     .str())
        .emit();
  }
}

} // namespace

void TypeConcretizer::visit(const HirNode *node) {
  // Base `visit` walks children first, so types resolve bottom-up.
  HirVisitor::visit(node);
  ctx.getTypeContext().concretize(node);

  // Range-check a literal that directly supplies a value: a binding
  // initializer, a returned value, or a call argument.
  if (const auto *let = LetStmt::cast(node)) {
    checkLiteralRange(let->getExpr(), ctx);
  } else if (const auto *ret = ReturnStmt::cast(node); ret && ret->getExpr()) {
    checkLiteralRange(ret->getExpr(), ctx);
  } else if (const auto *call = FnCallExpr::cast(node)) {
    for (const Expr *arg : call->getArgs()) {
      checkLiteralRange(arg, ctx);
    }
  }
}

} // namespace yuzu::hir
