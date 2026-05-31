#ifndef YUZU_HIR_TYPES_TYPEINFERRER_H
#define YUZU_HIR_TYPES_TYPEINFERRER_H

#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/HirContext.h"
#include "yuzu/Hir/HirVisitor.h"

namespace yuzu::hir {
/// Post-order walk that types every node (literals → holes, idents → their
/// decl, operators via `op->resolve`, `let`s checked). Children are typed
/// first; types may stay holes until `TypeConcretizer` resolves them.
class TypeInferrer final : public HirVisitor<TypeInferrer> {
public:
  explicit TypeInferrer(HirContext &ctx) : ctx(ctx) {}

  void visitBoolLit(const BoolLit *n);
  void visitStringLit(const StringLit *n);
  void visitIntLit(const IntLit *n);
  void visitFloatLit(const FloatLit *n);
  void visitIdentExpr(const IdentExpr *n);
  void visitCallExpr(const CallExpr *n);
  void visitLetStmt(const LetStmt *n);

private:
  HirContext &ctx;
};

} // namespace yuzu::hir

#endif // YUZU_HIR_TYPES_TYPEINFERRER_H