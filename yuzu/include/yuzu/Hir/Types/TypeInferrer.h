#ifndef YUZU_HIR_TYPES_TYPEINFERRER_H
#define YUZU_HIR_TYPES_TYPEINFERRER_H

#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/HirContext.h"
#include "yuzu/Hir/HirVisitor.h"
#include "yuzu/Types/Type.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallVector.h>

#include <string_view>

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
  void visitStructExpr(const StructExpr *n);
  void visitCallExpr(const CallExpr *n);
  void visitFuncCallExpr(const FuncCallExpr *n);
  void visitFieldAccessExpr(const FieldAccessExpr *n);
  void visitLetStmt(const LetStmt *n);
  void visitAssignStmt(const AssignStmt *n);
  void visitReturnStmt(const ReturnStmt *n);

  // Hoist function signatures before walking bodies, so a function can refer
  // to siblings declared later in the same scope (forward references).
  void traverseRoot(const Root *n);
  void traverseBlockStmt(const BlockStmt *n);

  // Override traversal so params bind into the fn scope before the body types.
  void traverseFuncStmt(const FuncStmt *n);

  // A query (`from … |> select …`) is typed as its own pass: a row scope is
  // pushed, the `from` resolves a table and binds the row alias, and each
  // `select` item types against that alias. So both query roots take over
  // traversal rather than letting the generic walk type the row exprs.
  void traverseFromRel(const FromRel *n);
  void traverseSelectRel(const SelectRel *n);
  void traverseWhereRel(const WhereRel *n);

private:
  /// Hoist declarations (structs → tables → functions) so the bodies that
  /// follow can reference any of them regardless of order, then walk/type each
  /// statement.
  void hoistAndWalk(llvm::ArrayRef<const Stmt *> stmts);

  /// Build the struct's `types::StructType` and bind its name into the type
  /// namespace.
  void registerStruct(const StructStmt *n);
  /// Resolve the table's row struct, make `Relation[Row]`, and register it in
  /// the table namespace (relational-only — never the value namespace).
  void registerTable(const TableStmt *n);

  /// Type a query expression, returning its `Relation[T]`. Recurses
  /// manually (not via the visitor) so the row scope spans `from` through
  /// `select`.
  const types::Type *inferQuery(const Expr *query);
  const types::Type *inferFromRel(const FromRel *n);
  const types::Type *inferSelectRel(const SelectRel *n);
  const types::Type *inferWhereRel(const WhereRel *n);

  /// A function's signature type, resolved on first request and memoized in
  /// the type side table. This is the lazy "query" that powers forward
  /// references: a call reading a not-yet-walked function's type triggers
  /// resolution here instead of relying on declaration order.
  const types::FuncType *signatureOf(const FuncStmt *n);

  /// Resolve the signature of `n` — type params, parameter types, and return
  /// type — binding each into the (already-pushed) function scope and the
  /// type side table. Returns the signature type.
  const types::FuncType *resolveFuncType(const FuncStmt *n);

  /// Process the `where` clause: register each bound trait as implemented by
  /// its type-parameter marker (so the body's operators resolve) and record
  /// the bounds for call-site checking. Type params must already be in
  /// scope.
  void resolveTraitBounds(const FuncStmt *funcStmt);

  /// The semantic type a written annotation denotes, resolved against scope;
  /// diagnostics anchor at `node`.
  const types::Type *resolveTypeAnnotation(const TypeAnnotation *type,
                                           const HirNode *node);
  const types::Type *
  resolveNamedTypeAnnotation(const NamedTypeAnnotation *annotation,
                             const HirNode *node);
  const types::Type *
  resolveFuncTypeAnnotation(const FuncTypeAnnotation *annotation,
                            const HirNode *node);

  /// The marker for a type-parameter declaration, interned by its declaration
  /// node. The same `[T]` always yields the same marker (so a signature and
  /// its body share it); distinct declarations yield distinct markers (so a
  /// nested `[U]` is never confused with an enclosing `[T]`, even at the same
  /// position).
  const types::TypeParamType *markerFor(const Ident *decl, uint32_t index);

  /// Substitute a type for one call site: each generic marker → a fresh hole
  /// (same marker → same hole, via `subst`, which is keyed by marker identity
  /// so a captured outer parameter isn't conflated with an own one).
  const types::Type *substituteType(
      const types::Type *type,
      llvm::DenseMap<const types::Type *, const types::Type *> &subst);

  /// Make `value` assignable to `target`: unify (so a literal adopts the
  /// target), else concretize and widen-coerce. False if neither applies —
  /// the caller emits its own diagnostic. Used for `let`, args, and returns.
  bool isAssignable(const Expr *value, const types::Type *target);

  HirContext &ctx;

  // Type-parameter markers interned by their declaration node, so each
  // `[T]` has one stable identity across signature resolution and body
  // checking.
  llvm::DenseMap<const Ident *, const types::TypeParamType *> typeParamMarkers;

  // The trait bounds of each generic marker (`where T: Add`), used to check at
  // a call site that the instantiating type actually implements them.
  llvm::DenseMap<const types::TypeParamType *,
                 llvm::SmallVector<std::u32string_view>>
      markerBounds;

  // Declared return type of the function being typed, null outside one.
  // Saved/restored across nested functions in `traverseFuncStmt`.
  const types::Type *expectedReturn = nullptr;
};

} // namespace yuzu::hir

#endif // YUZU_HIR_TYPES_TYPEINFERRER_H