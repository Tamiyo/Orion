#ifndef YUZU_HIR_TYPES_TYPEINFERRER_H
#define YUZU_HIR_TYPES_TYPEINFERRER_H

#include "yuzu/Ast/Ast.h"
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
  void visitReturnStmt(const ReturnStmt *n);

  // A function controls its own traversal: bind params into a fresh scope
  // *before* the body is typed (post-order `visit` would type the body
  // first, when params aren't yet in scope).
  void traverseFnStmt(const FnStmt *n);

private:
  /// The resolved type a node's annotation denotes, or null if it has none.
  /// Resolves the raw `ast::TypeExpr` recorded by lowering against the type
  /// scope (builtins today; `[T]` params once generics land). Diagnostics
  /// anchor at `node`, the annotated HIR node.
  const Type *resolveAnnotation(const HirNode *node);
  const Type *resolveType(ast::TypeExpr type, const HirNode *node);
  const Type *resolveNamedType(ast::NamedType type, const HirNode *node);

  HirContext &ctx;

  // The declared return type of the function currently being typed, or null
  // outside any function / when the function has no return annotation.
  // Saved and restored across nested functions in `traverseFnStmt`.
  const Type *expectedReturn = nullptr;
};

} // namespace yuzu::hir

#endif // YUZU_HIR_TYPES_TYPEINFERRER_H