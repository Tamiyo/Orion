#ifndef YUZU_ANF_REDUCTION_ANFREDUCER_H
#define YUZU_ANF_REDUCTION_ANFREDUCER_H

#include "yuzu/Anf/Anf.h"

#include <llvm/ADT/DenseMap.h>

#include <vector>

namespace yuzu::anf {

class AnfContext;

/// Reduces an ANF program by partial evaluation, rooted at the query. Each
/// select column is evaluated under an environment mapping bindings to their
/// already-reduced values, which fuses three transforms into one demand-driven
/// pass:
///   - constant folding (a call with constant operands becomes a constant);
///   - copy / constant propagation (a name resolves to its environment value);
///   - function inlining (a direct call evaluates the callee's body inline,
///     parameters bound to the argument atoms).
/// Only the lets a result genuinely needs are emitted, so dead temporaries are
/// never created and a constant flows straight into the expression that uses
/// it. Functions are templates consulted on demand; an uncalled one is left for
/// dead-code elimination.
class AnfReducer final {
public:
  explicit AnfReducer(AnfContext &ctx) : ctx(ctx) {}

  void reduce(const Root *root);

private:
  /// Binding -> its reduced value, for the evaluation in progress (a column, or
  /// one inlined call). A free name (a query's row alias) is absent and passes
  /// through unchanged.
  using Env = llvm::DenseMap<const Binding *, const Atom *>;

  /// Dispatch on the relation kind: a `FromRel` is the pipe source (nothing to
  /// reduce), a `SelectRel` recurses into its input and reduces its columns, a
  /// `WhereRel` recurses and reduces its predicate.
  void reduceRel(const Rel *rel);
  void reduceSelectRel(const SelectRel *select);
  void reduceSelectItem(const SelectItem *item);
  void reduceWhereRel(const WhereRel *where);
  void reduceDistinctRel(const DistinctRel *distinct);
  void reduceDropRel(const DropRel *drop);
  void reduceRenameRel(const RenameRel *rename);
  void reduceExtendRel(const ExtendRel *extend);

  /// Reduce a column/predicate `Thunk`: evaluate its block, cap it with the
  /// reduced tail, and return the rebuilt thunk.
  const Thunk *reduceThunk(const Thunk *body);

  /// Drop function definitions the reduced program no longer calls. A `FuncRef`
  /// keeps its target alive (transitively); after full inlining the query holds
  /// none, so every inlined function is removed.
  void eliminateDeadFunctions(const Root *root);

  /// Evaluate a statement sequence under `env`, appending the lets it needs to
  /// the `intermediateStmts` buffer, and return the atom its tail (`return` /
  /// tail expression) yields. Serves both a column `Thunk` and an inlined
  /// function `BlockStmt`.
  const Atom *reduceBlock(llvm::ArrayRef<const Stmt *> stmts, Env &env,
                          unsigned depth);

  /// Evaluate one expression under `env` to an atom, appending any computation
  /// lets to `intermediateStmts`. `depth` bounds inlining recursion. Dispatches
  /// on the expression kind to the `reduce*` helper below.
  const Atom *reduce(const Expr *expr, Env &env, unsigned depth);

  /// A trivial atom: a constant or function reference is itself, a name
  /// resolves through `env`, a field access reduces its base.
  const Atom *reduceAtom(const Atom *atom, Env &env, unsigned depth);

  /// A builtin call: reduce the operands, then fold to a constant if it can,
  /// else emit the call as a let.
  const Atom *reduceCallExpr(const CallExpr *call, Env &env, unsigned depth);

  /// A function call: a direct call to a known function is inlined; an indirect
  /// or depth-capped call is emitted as a let.
  const Atom *reduceFuncCallExpr(const FuncCallExpr *call, Env &env,
                                 unsigned depth);

  /// A struct literal: reduce each field value, then emit the construction.
  const Atom *reduceStructExpr(const StructExpr *expr, Env &env,
                               unsigned depth);

  /// A list literal: reduce each element, then emit the construction.
  const Atom *reduceListExpr(const ListExpr *expr, Env &env, unsigned depth);

  /// Restore the ANF invariant for a non-atomic `computation`: bind it to a
  /// fresh temporary (a `let` appended to `intermediateStmts`) and return a
  /// `VarAtom` referencing that name, so a caller can use it as an operand.
  const Atom *bindToTemp(const Expr *computation, const types::Type *type);

  /// Reduction rewrites the program in place. ANF nodes are handed out as const
  /// views, so the in-place edits (capping a column with its reduced thunk,
  /// dropping dead functions) funnel through this one `const_cast` rather than
  /// scattering casts across the pass.
  template <typename T> static T *mutate(const T *node) {
    return const_cast<T *>(node);
  }

  AnfContext &ctx;

  /// Lets produced while reducing the current column, in order. Reset at the
  /// start of each column and sealed into its `Thunk`; mirrors the lowerer's
  /// `intermediateStmts`.
  std::vector<const Stmt *> intermediateStmts;
};

} // namespace yuzu::anf

#endif // YUZU_ANF_REDUCTION_ANFREDUCER_H
