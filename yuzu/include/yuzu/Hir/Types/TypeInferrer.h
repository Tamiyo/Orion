#ifndef YUZU_HIR_TYPES_TYPEINFERRER_H
#define YUZU_HIR_TYPES_TYPEINFERRER_H

#include "yuzu/Ast/Ast.h"
#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/HirContext.h"
#include "yuzu/Hir/HirVisitor.h"

#include <llvm/ADT/DenseMap.h>

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
  void visitFnCallExpr(const FnCallExpr *n);
  void visitLetStmt(const LetStmt *n);
  void visitReturnStmt(const ReturnStmt *n);

  // Override traversal so params bind into the fn scope before the body types.
  void traverseFnStmt(const FnStmt *n);

private:
  /// The type a node's annotation denotes, or null if it has none. Resolves
  /// the raw `ast::TypeExpr` against scope; diagnostics anchor at `node`.
  const Type *resolveAnnotation(const HirNode *node);
  const Type *resolveType(ast::TypeExpr type, const HirNode *node);
  const Type *resolveNamedType(ast::NamedType type, const HirNode *node);

  /// Instantiate a generic signature for one call: each `TypeParamTy` →
  /// a fresh hole (same index → same hole, via `subst`).
  const Type *instantiate(const Type *type,
                          llvm::DenseMap<uint32_t, const Type *> &subst);

  HirContext &ctx;

  // Declared return type of the function being typed, null outside one.
  // Saved/restored across nested functions in `traverseFnStmt`.
  const Type *expectedReturn = nullptr;
};

} // namespace yuzu::hir

#endif // YUZU_HIR_TYPES_TYPEINFERRER_H