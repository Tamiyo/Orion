#ifndef YUZU_ANF_REDUCTION_ANFREDUCER_H
#define YUZU_ANF_REDUCTION_ANFREDUCER_H

#include "yuzu/Anf/Anf.h"
#include "yuzu/Anf/AnfVisitor.h"

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

private:
  /// Follow `VarAtom -> Binding -> value` to the constant or copy it ultimately
  /// names; returns `atom` unchanged when it names a real computation, a
  /// projection, or a parameter.
  const Atom *resolveAtom(const Atom *atom);

  AnfContext &ctx;

  /// Set when a pass rewrites something, so `reduce` knows to iterate again.
  bool changed = false;
};

} // namespace yuzu::anf

#endif // YUZU_ANF_REDUCTION_ANFREDUCER_H
