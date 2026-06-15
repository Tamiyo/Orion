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
constexpr unsigned kMaxInlineDepth = 256;
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
}

void AnfReducer::reduceRel(const Rel *rel) {
  const auto *select = SelectRel::cast(rel);
  if (select == nullptr) {
    return; // `FromRel` is the pipe source; nothing to reduce.
  }
  if (const auto *input = Rel::cast(select->getInput())) {
    reduceRel(input);
  }
  for (const auto *item : select->getItems()) {
    reduceSelectItem(item);
  }
}

void AnfReducer::reduceSelectItem(const SelectItem *item) {
  // A column starts with an empty environment (its only free name is the row
  // alias, which passes through). Evaluate the body, then cap it with its tail.
  Env env;
  std::vector<const Stmt *> out;
  const Atom *tail = reduceBlock(item->getBody(), env, out, /*depth=*/0);
  if (tail != nullptr) {
    out.push_back(ctx.getBuilder().makeExprStmt(tail));
  }
  const_cast<SelectItem *>(item)->setBody(ctx.getBuilder().makeBlockStmt(out));
}

const Atom *AnfReducer::reduceBlock(const BlockStmt *block, Env &env,
                                    std::vector<const Stmt *> &out,
                                    unsigned depth) {
  const Atom *tail = nullptr;
  for (const Stmt *stmt : block->getStmts()) {
    if (const auto *let = LetStmt::cast(stmt)) {
      // Bind the local to its reduced value; the value is only emitted if it's
      // a real computation (otherwise it's a constant/copy folded into uses).
      env[let->getBinding()] =
          reduce(let->getBinding()->getValue(), env, out, depth);
    } else if (const auto *ret = ReturnStmt::cast(stmt)) {
      tail = ret->getValue() != nullptr
                 ? reduce(ret->getValue(), env, out, depth)
                 : nullptr;
    } else if (const auto *exprStmt = ExprStmt::cast(stmt)) {
      tail = reduce(exprStmt->getValue(), env, out, depth);
    }
  }
  return tail;
}

const Atom *AnfReducer::reduce(const Expr *expr, Env &env,
                               std::vector<const Stmt *> &out, unsigned depth) {
  // Trivial atoms: a constant is itself; a name resolves through the
  // environment (its reduced value), or passes through if free.
  if (const auto *constant = Constant::cast(expr)) {
    return constant;
  }
  if (const auto *ref = FuncRef::cast(expr)) {
    return ref;
  }
  if (const auto *var = VarAtom::cast(expr)) {
    const auto it = env.find(var->getBinding());
    return it != env.end() ? it->second : var;
  }
  if (const auto *field = FieldAtom::cast(expr)) {
    const Atom *base = reduce(field->getBase(), env, out, depth);
    return ctx.getBuilder().makeFieldAtom(base, field->getField(),
                                          field->getType());
  }

  // A builtin call: reduce operands, fold if they're all constant, else emit.
  if (const auto *call = CallExpr::cast(expr)) {
    llvm::SmallVector<const Atom *, 4> args;
    for (const Atom *arg : call->getArgs()) {
      args.push_back(reduce(arg, env, out, depth));
    }
    if (const Constant *folded =
            call->getOp()->fold(args, ctx, call->getType())) {
      return folded;
    }
    return emit(
        ctx.getBuilder().makeCallExpr(call->getOp(), args, call->getType()),
        call->getType(), out);
  }

  // A function call: a direct call evaluates the callee's body inline; an
  // indirect or depth-capped call is kept.
  if (const auto *funcCall = FuncCallExpr::cast(expr)) {
    llvm::SmallVector<const Atom *, 4> args;
    for (const Atom *arg : funcCall->getArgs()) {
      args.push_back(reduce(arg, env, out, depth));
    }
    if (const auto *ref = FuncRef::cast(funcCall->getCallee())) {
      if (depth < kMaxInlineDepth) {
        const FuncStmt *func = ref->getFunc();
        const auto params = func->getParams();
        if (params.size() == args.size()) {
          Env callEnv;
          for (std::size_t i = 0; i < params.size(); ++i) {
            callEnv[params[i]->getBinding()] = args[i];
          }
          return reduceBlock(func->getBody(), callEnv, out, depth + 1);
        }
      } else {
        ctx.warning(funcCall,
                    "inlining depth limit reached; call left in place")
            .emit();
      }
    }
    const Atom *callee = reduce(funcCall->getCallee(), env, out, depth);
    return emit(
        ctx.getBuilder().makeFuncCallExpr(callee, args, funcCall->getType()),
        funcCall->getType(), out);
  }

  // A struct literal: reduce field values, then emit the construction.
  if (const auto *structExpr = StructExpr::cast(expr)) {
    llvm::SmallVector<const StructFieldInit *, 4> fields;
    for (const auto *field : structExpr->getFields()) {
      fields.push_back(ctx.getBuilder().makeStructFieldInit(
          field->getName(), reduce(field->getValue(), env, out, depth)));
    }
    return emit(ctx.getBuilder().makeStructExpr(structExpr->getName(), fields,
                                                structExpr->getType()),
                structExpr->getType(), out);
  }

  util::yuzu_unreachable("unexpected expression kind while reducing");
}

const Atom *AnfReducer::emit(const Expr *computation, const types::Type *type,
                             std::vector<const Stmt *> &out) {
  const Binding *binding =
      ctx.getBuilder().makeBinding(makeTemp(), computation, type);
  out.push_back(ctx.getBuilder().makeLetStmt(binding));
  return ctx.getBuilder().makeVarAtom(binding, type);
}

const Ident *AnfReducer::makeTemp() {
  std::u32string name = U"%t";
  for (char c : std::to_string(tempCounter++)) {
    name.push_back(static_cast<char32_t>(c));
  }
  return ctx.getBuilder().makeIdent(ctx.getStringInterner().intern(name));
}

} // namespace yuzu::anf
