#include "yuzu/Anf/Reduction/AnfReducer.h"

#include "yuzu/Anf/Anf.h"
#include "yuzu/Anf/AnfContext.h"
#include "yuzu/Anf/Ops/Op.h"

#include <llvm/ADT/SmallVector.h>

namespace yuzu::anf {

void AnfReducer::reduce(const Root *root) {
  // Folding can expose further folds, so walk to a fixpoint.
  do {
    changed = false;
    visit(root);
  } while (changed);
}

void AnfReducer::visitBinding(const Binding *binding) {
  const Expr *value = binding->getValue();
  if (value == nullptr) {
    return; // A parameter binding: no defining value.
  }

  // The default walk already reduced this binding's value subtree; now fold a
  // builtin call whose operands all resolve to constants.
  const auto *call = CallExpr::cast(value);
  if (call == nullptr) {
    return;
  }

  llvm::SmallVector<const Atom *, 4> args;
  args.reserve(call->getArgs().size());
  for (const Atom *arg : call->getArgs()) {
    args.push_back(resolveAtom(arg));
  }

  if (const Constant *folded =
          call->getOp()->fold(args, ctx, call->getType())) {
    // The reducer is the one authorized mutator of the otherwise-const tree:
    // overwrite the binding's value, and let later reads resolve through to it.
    const_cast<Binding *>(binding)->setValue(folded);
    changed = true;
  }
}

const Atom *AnfReducer::resolveAtom(const Atom *atom) {
  const auto *var = VarAtom::cast(atom);
  if (var == nullptr) {
    return atom;
  }

  const Binding *binding = var->getBinding();
  if (binding == nullptr) {
    return atom; // Unresolved reference.
  }

  const Expr *value = binding->getValue();
  if (value == nullptr) {
    return atom; // A parameter.
  }

  // Only look through to a freely-duplicable value: a constant or another var.
  // A projection (`FieldAtom`) or a real computation stays named by its
  // binding, so we neither duplicate work nor lose a name.
  const auto *inner = Atom::cast(value);
  if (inner == nullptr || FieldAtom::cast(inner) != nullptr) {
    return atom;
  }

  return resolveAtom(inner); // Chase copy chains.
}

} // namespace yuzu::anf
