#include "yuzu/Anf/Reduction/AnfReducer.h"

#include "yuzu/Anf/Anf.h"
#include "yuzu/Anf/AnfContext.h"
#include "yuzu/Anf/Ops/Op.h"
#include "yuzu/Util/ErrorHandling.h"

#include <llvm/ADT/SmallVector.h>

#include <cstddef>
#include <string>

namespace yuzu::anf {

namespace {
/// Recursion guard for inlining: a self- or mutually-recursive call expands
/// this many levels deep, then the innermost call is left in place (warned).
constexpr unsigned kMaxInlineDepth = 16384;

/// The direct-call target of `value`, or null if it isn't a `FuncRef` call.
const FuncCallExpr *asDirectCall(const Expr *value) {
  const auto *call = FuncCallExpr::cast(value);
  if (call != nullptr && FuncRef::cast(call->getCallee()) != nullptr) {
    return call;
  }
  return nullptr;
}
} // namespace

void AnfReducer::visitSelectItem(const SelectItem *item) {
  inlineBlock(item->getBody());
}

void AnfReducer::inlineBlock(const BlockStmt *block) {
  std::vector<const Stmt *> &stmts =
      const_cast<BlockStmt *>(block)->getStmtsMutable();

  for (std::size_t i = 0; i < stmts.size();) {
    // A direct call evaluated by this statement — bound by a `let`, or the
    // column's tail `ExprStmt`.
    const auto *let = LetStmt::cast(stmts[i]);
    const auto *exprStmt = ExprStmt::cast(stmts[i]);
    const FuncCallExpr *call = nullptr;
    if (let != nullptr) {
      call = asDirectCall(let->getBinding()->getValue());
    } else if (exprStmt != nullptr) {
      call = asDirectCall(exprStmt->getValue());
    }
    if (call == nullptr) {
      ++i;
      continue;
    }

    // Build the callee's inlined body, then splice it in just before this
    // statement, whose value slot now names the callee's return value.
    std::vector<const Stmt *> body;
    const Atom *returnAtom = inlineCall(call, 0, body);
    if (returnAtom == nullptr) {
      ++i; // Not inlinable (e.g. arity mismatch): leave the call in place.
      continue;
    }

    if (let != nullptr) {
      const_cast<Binding *>(let->getBinding())->setValue(returnAtom);
    } else {
      const_cast<ExprStmt *>(exprStmt)->setValue(returnAtom);
    }
    stmts.insert(stmts.begin() + i, body.begin(), body.end());
    i += body.size() + 1; // Past the spliced body and the rebound statement.
    changed = true;
  }
}

const Atom *AnfReducer::inlineCall(const FuncCallExpr *call, unsigned depth,
                                   std::vector<const Stmt *> &out) {
  const FuncStmt *func = FuncRef::cast(call->getCallee())->getFunc();
  const auto params = func->getParams();
  const auto args = call->getArgs();
  if (params.size() != args.size()) {
    return nullptr; // Arity mismatch — shouldn't happen post-typecheck.
  }

  // Each parameter stands for the matching argument atom at this call site.
  InlineEnv env;
  for (std::size_t i = 0; i < params.size(); ++i) {
    env.substitution[params[i]->getBinding()] = args[i];
  }

  const Atom *returnAtom = nullptr;
  for (const Stmt *stmt : func->getBody()->getStmts()) {
    if (const auto *ret = ReturnStmt::cast(stmt)) {
      returnAtom = ret->getValue() != nullptr ? cloneAtom(ret->getValue(), env)
                                              : nullptr;
      continue;
    }

    const auto *let = LetStmt::cast(stmt);
    if (let == nullptr) {
      out.push_back(cloneStmt(stmt, env));
      continue;
    }

    const Binding *cloned = cloneBinding(let->getBinding(), env);
    // Expand a nested direct call, unless that would recurse past the cap.
    if (const FuncCallExpr *nested = asDirectCall(cloned->getValue())) {
      if (depth + 1 < kMaxInlineDepth) {
        if (const Atom *nestedReturn = inlineCall(nested, depth + 1, out)) {
          const_cast<Binding *>(cloned)->setValue(nestedReturn);
        }
      } else {
        ctx.warning(cloned, "inlining depth limit reached; call left in place")
            .emit();
      }
    }
    out.push_back(ctx.getBuilder().makeLetStmt(cloned));
  }
  return returnAtom;
}

const Stmt *AnfReducer::cloneStmt(const Stmt *stmt, InlineEnv &env) {
  if (const auto *let = LetStmt::cast(stmt)) {
    return ctx.getBuilder().makeLetStmt(cloneBinding(let->getBinding(), env));
  }
  if (const auto *exprStmt = ExprStmt::cast(stmt)) {
    return ctx.getBuilder().makeExprStmt(cloneExpr(exprStmt->getValue(), env));
  }
  if (const auto *ret = ReturnStmt::cast(stmt)) {
    return ctx.getBuilder().makeReturnStmt(
        ret->getValue() != nullptr ? cloneAtom(ret->getValue(), env) : nullptr);
  }
  util::yuzu_unreachable("unexpected statement kind while inlining");
}

const Binding *AnfReducer::cloneBinding(const Binding *binding,
                                        InlineEnv &env) {
  const Expr *value = binding->getValue();
  const Expr *clonedValue = value != nullptr ? cloneExpr(value, env) : nullptr;
  const Binding *cloned =
      ctx.getBuilder().makeBinding(makeTemp(), clonedValue, binding->getType());
  // Later uses of the original local now resolve to this fresh copy.
  env.remap[binding] = cloned;
  return cloned;
}

const Expr *AnfReducer::cloneExpr(const Expr *expr, InlineEnv &env) {
  if (const auto *call = CallExpr::cast(expr)) {
    llvm::SmallVector<const Atom *, 4> args;
    for (const Atom *arg : call->getArgs()) {
      args.push_back(cloneAtom(arg, env));
    }
    return ctx.getBuilder().makeCallExpr(call->getOp(), args, call->getType());
  }
  if (const auto *call = FuncCallExpr::cast(expr)) {
    llvm::SmallVector<const Atom *, 4> args;
    for (const Atom *arg : call->getArgs()) {
      args.push_back(cloneAtom(arg, env));
    }
    return ctx.getBuilder().makeFuncCallExpr(cloneAtom(call->getCallee(), env),
                                             args, call->getType());
  }
  if (const auto *structExpr = StructExpr::cast(expr)) {
    llvm::SmallVector<const StructFieldInit *, 4> fields;
    for (const auto *field : structExpr->getFields()) {
      fields.push_back(ctx.getBuilder().makeStructFieldInit(
          field->getName(), cloneAtom(field->getValue(), env)));
    }
    return ctx.getBuilder().makeStructExpr(structExpr->getName(), fields,
                                           structExpr->getType());
  }
  // The remaining expressions are atoms (a callee body has no relational ops).
  if (const auto *atom = Atom::cast(expr)) {
    return cloneAtom(atom, env);
  }
  util::yuzu_unreachable("unexpected expression kind while inlining");
}

const Atom *AnfReducer::cloneAtom(const Atom *atom, InlineEnv &env) {
  if (const auto *var = VarAtom::cast(atom)) {
    const Binding *binding = var->getBinding();
    // A parameter resolves to the call's argument atom.
    if (const auto it = env.substitution.find(binding);
        it != env.substitution.end()) {
      return it->second;
    }
    // A cloned local resolves to its fresh copy.
    if (const auto it = env.remap.find(binding); it != env.remap.end()) {
      return ctx.getBuilder().makeVarAtom(it->second, var->getType());
    }
    // Anything else (not expected in a self-contained body) is left as-is.
    return atom;
  }
  if (const auto *field = FieldAtom::cast(atom)) {
    return ctx.getBuilder().makeFieldAtom(cloneAtom(field->getBase(), env),
                                          field->getField(), field->getType());
  }
  // Constants and `FuncRef`s hold no binding references; reuse them.
  return atom;
}

const Ident *AnfReducer::makeTemp() {
  std::u32string name = U"%t";
  for (char c : std::to_string(tempCounter++)) {
    name.push_back(static_cast<char32_t>(c));
  }
  return ctx.getBuilder().makeIdent(ctx.getStringInterner().intern(name));
}

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
