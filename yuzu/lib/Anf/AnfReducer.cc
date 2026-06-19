#include "yuzu/Anf/Reduction/AnfReducer.h"

#include "yuzu/Anf/Anf.h"
#include "yuzu/Anf/AnfContext.h"
#include "yuzu/Anf/AnfVisitor.h"
#include "yuzu/Anf/Ops/OpFold.h"
#include "yuzu/Util/ErrorHandling.h"

#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/SmallVector.h>

#include <algorithm>
#include <cstddef>
#include <vector>

namespace yuzu::anf {

namespace {
/// Recursion guard for inlining: a self- or mutually-recursive call expands
/// this many levels deep, then the innermost call is left in place (warned).
constexpr unsigned kMaxInlineDepth = 256;

/// Walks a subtree gathering the functions its `FuncRef`s name, newly-found
/// ones queued for transitive reachability.
struct FuncRefCollector : AnfVisitor<FuncRefCollector> {
  FuncRefCollector(llvm::DenseSet<const FuncStmt *> &live,
                   std::vector<const FuncStmt *> &worklist)
      : live(live), worklist(worklist) {}

  void visitFuncRef(FuncRef *ref) {
    const FuncStmt *func = ref->getFunc();
    if (func != nullptr && live.insert(func).second) {
      worklist.push_back(func);
    }
  }

  llvm::DenseSet<const FuncStmt *> &live;
  std::vector<const FuncStmt *> &worklist;
};
} // namespace

void AnfReducer::reduce(const Root *root) {
  // Reduction is rooted at the query: walk the program for the relational
  // expression and reduce its columns. Functions are reached on demand when a
  // column calls them.
  for (const Stmt *stmt : root->getStmts()) {
    if (const auto *exprStmt = ExprStmt::cast(stmt)) {
      if (const auto *rel = Rel::cast(exprStmt->getValue())) {
        reduceRel(rel);
      }
    }
  }

  eliminateDeadFunctions(root);
}

void AnfReducer::eliminateDeadFunctions(const Root *root) {
  llvm::DenseSet<const FuncStmt *> live;
  std::vector<const FuncStmt *> worklist;
  FuncRefCollector collector(live, worklist);

  // Seed from everything that isn't a function definition (the query), then
  // transitively keep functions referenced by live ones.
  for (const Stmt *stmt : root->getStmts()) {
    if (FuncStmt::cast(stmt) == nullptr) {
      collector.visit(mutate(stmt));
    }
  }
  while (!worklist.empty()) {
    const FuncStmt *func = worklist.back();
    worklist.pop_back();
    collector.visit(mutate(func->getBody()));
  }

  // Erase the function definitions nothing references anymore.
  std::vector<const Stmt *> &stmts = mutate(root)->getStmtsMutable();
  stmts.erase(std::remove_if(stmts.begin(), stmts.end(),
                             [&](const Stmt *stmt) {
                               const auto *func = FuncStmt::cast(stmt);
                               return func != nullptr && !live.contains(func);
                             }),
              stmts.end());
}

void AnfReducer::reduceRel(const Rel *rel) {
  switch (rel->getRelKind()) {
  case RelKind::FromRel:
    // The pipe source; nothing to reduce.
    return;
  case RelKind::SelectRel:
    return reduceSelectRel(SelectRel::cast(rel));
  case RelKind::WhereRel:
    return reduceWhereRel(WhereRel::cast(rel));
  case RelKind::DistinctRel:
    return reduceDistinctRel(DistinctRel::cast(rel));
  case RelKind::DropRel:
    return reduceDropRel(DropRel::cast(rel));
  case RelKind::RenameRel:
    return reduceRenameRel(RenameRel::cast(rel));
  }
}

void AnfReducer::reduceSelectRel(const SelectRel *select) {
  if (const auto *input = Rel::cast(select->getInput())) {
    reduceRel(input);
  }
  for (const auto *item : select->getItems()) {
    reduceSelectItem(item);
  }
}

void AnfReducer::reduceWhereRel(const WhereRel *where) {
  if (const auto *input = Rel::cast(where->getInput())) {
    reduceRel(input);
  }
  mutate(where)->setPredicate(reduceThunk(where->getPredicate()));
}

void AnfReducer::reduceDistinctRel(const DistinctRel *distinct) {
  if (const auto *input = Rel::cast(distinct->getInput())) {
    reduceRel(input);
  }
}

void AnfReducer::reduceDropRel(const DropRel *drop) {
  if (const auto *input = Rel::cast(drop->getInput())) {
    reduceRel(input);
  }
}

void AnfReducer::reduceRenameRel(const RenameRel *rename) {
  if (const auto *input = Rel::cast(rename->getInput())) {
    reduceRel(input);
  }
}

const Thunk *AnfReducer::reduceThunk(const Thunk *body) {
  // A column/predicate starts with an empty environment (its only free name is
  // the row alias, which passes through) and a fresh emit buffer. Evaluate the
  // block, then cap it with its reduced tail.
  intermediateStmts.clear();
  Env env;
  const Atom *tail = reduceBlock(body->getStmts(), env, /*depth=*/0);
  if (tail != nullptr) {
    intermediateStmts.push_back(ctx.getBuilder().makeExprStmt(tail));
  }
  return ctx.getBuilder().makeThunk(intermediateStmts);
}

void AnfReducer::reduceSelectItem(const SelectItem *item) {
  mutate(item)->setBody(reduceThunk(item->getBody()));
}

const Atom *AnfReducer::reduceBlock(llvm::ArrayRef<const Stmt *> stmts,
                                    Env &env, unsigned depth) {
  const Atom *tail = nullptr;
  for (const Stmt *stmt : stmts) {
    if (const auto *let = LetStmt::cast(stmt)) {
      // Bind the local to its reduced value; the value is only emitted if it's
      // a real computation (otherwise it's a constant/copy folded into uses).
      env[let->getBinding()] =
          reduce(let->getBinding()->getValue(), env, depth);
    } else if (const auto *ret = ReturnStmt::cast(stmt)) {
      tail = ret->getValue() != nullptr ? reduce(ret->getValue(), env, depth)
                                        : nullptr;
    } else if (const auto *exprStmt = ExprStmt::cast(stmt)) {
      tail = reduce(exprStmt->getValue(), env, depth);
    }
  }
  return tail;
}

const Atom *AnfReducer::reduce(const Expr *expr, Env &env, unsigned depth) {
  switch (expr->getExprKind()) {
  case ExprKind::Atom:
    return reduceAtom(Atom::cast(expr), env, depth);
  case ExprKind::CallExpr:
    return reduceCallExpr(CallExpr::cast(expr), env, depth);
  case ExprKind::FuncCallExpr:
    return reduceFuncCallExpr(FuncCallExpr::cast(expr), env, depth);
  case ExprKind::StructExpr:
    return reduceStructExpr(StructExpr::cast(expr), env, depth);
  case ExprKind::Rel:
    util::yuzu_unreachable("a relational expression cannot appear in a column");
  }
}

const Atom *AnfReducer::reduceAtom(const Atom *atom, Env &env, unsigned depth) {
  switch (atom->getAtomKind()) {
  case AtomKind::Constant:
    // A constant evaluates to itself.
    return Constant::cast(atom);
  case AtomKind::FuncRef:
    // A function reference passes through unchanged.
    return FuncRef::cast(atom);
  case AtomKind::VarAtom: {
    // A name resolves to its reduced value, or passes through if it is free
    // (the query's row alias).
    const auto *var = VarAtom::cast(atom);
    const auto it = env.find(var->getBinding());
    return it != env.end() ? it->second : var;
  }
  case AtomKind::FieldAtom: {
    // A field access reduces its base, then rebuilds against the result.
    const auto *field = FieldAtom::cast(atom);
    const Atom *base = reduce(field->getBase(), env, depth);
    return ctx.getBuilder().makeFieldAtom(base, field->getField(),
                                          field->getType());
  }
  }
}

const Atom *AnfReducer::reduceCallExpr(const CallExpr *call, Env &env,
                                       unsigned depth) {
  llvm::SmallVector<const Atom *, 4> args;
  for (const Atom *arg : call->getArgs()) {
    args.push_back(reduce(arg, env, depth));
  }

  if (const Constant *folded =
          fold(call->getOp(), args, ctx, call->getType())) {
    return folded;
  }

  return bindToTemp(
      ctx.getBuilder().makeCallExpr(call->getOp(), args, call->getType()),
      call->getType());
}

const Atom *AnfReducer::reduceFuncCallExpr(const FuncCallExpr *call, Env &env,
                                           unsigned depth) {
  llvm::SmallVector<const Atom *, 4> args;
  for (const Atom *arg : call->getArgs()) {
    args.push_back(reduce(arg, env, depth));
  }

  // A direct call to a known function is inlined: its body is evaluated under
  // an environment binding the parameters to the argument atoms. An indirect
  // call, an arity mismatch, or a call past the depth limit is kept in place.
  if (const auto *ref = FuncRef::cast(call->getCallee())) {
    const FuncStmt *func = ref->getFunc();
    if (depth >= kMaxInlineDepth) {
      ctx.warning(call, "inlining depth limit reached; call left in place")
          .emit();
    } else if (const auto params = func->getParams();
               params.size() == args.size()) {
      Env callEnv;
      for (std::size_t i = 0; i < params.size(); ++i) {
        callEnv[params[i]->getBinding()] = args[i];
      }
      return reduceBlock(func->getBody()->getStmts(), callEnv, depth + 1);
    }
  }

  const Atom *callee = reduce(call->getCallee(), env, depth);
  return bindToTemp(
      ctx.getBuilder().makeFuncCallExpr(callee, args, call->getType()),
      call->getType());
}

const Atom *AnfReducer::reduceStructExpr(const StructExpr *expr, Env &env,
                                         unsigned depth) {
  llvm::SmallVector<const StructFieldInit *, 4> fields;
  for (const auto *field : expr->getFields()) {
    fields.push_back(ctx.getBuilder().makeStructFieldInit(
        field->getName(), reduce(field->getValue(), env, depth)));
  }

  return bindToTemp(
      ctx.getBuilder().makeStructExpr(expr->getName(), fields, expr->getType()),
      expr->getType());
}

const Atom *AnfReducer::bindToTemp(const Expr *computation,
                                   const types::Type *type) {
  const Binding *binding =
      ctx.getBuilder().makeBinding(ctx.makeTemp(), computation, type);
  intermediateStmts.push_back(ctx.getBuilder().makeLetStmt(binding));
  return ctx.getBuilder().makeVarAtom(binding, type);
}

} // namespace yuzu::anf
