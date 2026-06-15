#ifndef YUZU_ANF_REDUCTION_ANFREDUCER_H
#define YUZU_ANF_REDUCTION_ANFREDUCER_H

#include "yuzu/Anf/Anf.h"

#include <llvm/ADT/DenseMap.h>

#include <cstdint>
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

  void reduceRel(const Rel *rel);
  void reduceSelectItem(const SelectItem *item);

  /// Drop function definitions the reduced program no longer calls. A `FuncRef`
  /// keeps its target alive (transitively); after full inlining the query holds
  /// none, so every inlined function is removed.
  void eliminateDeadFunctions(const Root *root);

  /// Evaluate a statement sequence under `env`, appending the lets it needs to
  /// `out`, and return the atom its tail (`return` / tail expression) yields.
  const Atom *reduceBlock(const BlockStmt *block, Env &env,
                          std::vector<const Stmt *> &out, unsigned depth);

  /// Evaluate one expression under `env` to an atom, appending any computation
  /// lets to `out`. `depth` bounds inlining recursion.
  const Atom *reduce(const Expr *expr, Env &env, std::vector<const Stmt *> &out,
                     unsigned depth);

  /// Bind `computation` to a fresh temporary appended to `out`, and return a
  /// use of it.
  const Atom *emit(const Expr *computation, const types::Type *type,
                   std::vector<const Stmt *> &out);

  /// A fresh `%t` ident; only uniqueness matters (resolution is by pointer).
  const Ident *makeTemp();

  AnfContext &ctx;
  uint32_t tempCounter = 0;
};

} // namespace yuzu::anf

#endif // YUZU_ANF_REDUCTION_ANFREDUCER_H
