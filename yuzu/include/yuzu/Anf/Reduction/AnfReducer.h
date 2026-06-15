#ifndef YUZU_ANF_REDUCTION_ANFREDUCER_H
#define YUZU_ANF_REDUCTION_ANFREDUCER_H

#include "yuzu/Anf/Anf.h"
#include "yuzu/Anf/AnfVisitor.h"

#include <llvm/ADT/DenseMap.h>

#include <cstdint>
#include <vector>

namespace yuzu::anf {

class AnfContext;

/// Reduces an ANF program in place, iterated to a fixpoint. Built on the
/// generated `AnfVisitor`, whose default walk reaches every binding — function
/// bodies and query columns alike — so `visitBinding` is the only hook:
///   - constant / copy propagation is resolved by following the
///     `VarAtom -> Binding` back-edge at read time (`resolveAtom`), no
///     mutation;
///   - constant folding turns a call with constant operands into a constant via
///     the op's `fold`, overwriting the binding's value.
class AnfReducer final : public AnfVisitor<AnfReducer> {
public:
  explicit AnfReducer(AnfContext &ctx) : ctx(ctx) {}

  void reduce(const Root *root);

  /// Visitor hook: fold this binding's value when its operands are constant.
  void visitBinding(const Binding *binding);

  // Visitor hook: rebuild a query column's body, expanding direct calls inline.
  // (Runs after the default walk, so the body's bindings are already folded.)
  // The query is the inlining target; nested calls are expanded recursively by
  // `inlineCall`, so source functions need no in-place rewrite.
  void visitSelectItem(const SelectItem *item);

private:
  /// Per-inline rename state: function parameters map to the call's argument
  /// atoms; each cloned local binding maps to its fresh copy, so a cloned
  /// `VarAtom` points at the clone rather than the original.
  struct InlineEnv {
    llvm::DenseMap<const Binding *, const Atom *> substitution;
    llvm::DenseMap<const Binding *, const Binding *> remap;
  };

  /// Follow `VarAtom -> Binding -> value` to the constant or copy it ultimately
  /// names; returns `atom` unchanged when it names a real computation, a
  /// projection, or a parameter.
  const Atom *resolveAtom(const Atom *atom);

  /// Splice each `let x = f(args)` (direct call) in `block` open, replacing it
  /// with the callee's body inline. Edits the block's statement list in place.
  void inlineBlock(const BlockStmt *block);

  /// Clone the callee's body for one call site into `out` (fresh names, params
  /// substituted, nested direct calls expanded up to the depth cap) and return
  /// the cloned return atom. Null if the call can't be inlined.
  const Atom *inlineCall(const FuncCallExpr *call, unsigned depth,
                         std::vector<const Stmt *> &out);

  // Deep-copy a callee node, renaming through `env`.
  const Stmt *cloneStmt(const Stmt *stmt, InlineEnv &env);
  const Binding *cloneBinding(const Binding *binding, InlineEnv &env);
  const Expr *cloneExpr(const Expr *expr, InlineEnv &env);
  const Atom *cloneAtom(const Atom *atom, InlineEnv &env);

  /// A fresh `%t` ident for a cloned binding.
  const Ident *makeTemp();

  AnfContext &ctx;

  /// Set when a pass rewrites something, so `reduce` knows to iterate again.
  bool changed = false;

  /// Names cloned bindings; only uniqueness matters (resolution is by pointer).
  uint32_t tempCounter = 0;
};

} // namespace yuzu::anf

#endif // YUZU_ANF_REDUCTION_ANFREDUCER_H
