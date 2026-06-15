#include "yuzu/Anf/AnfLowerer.h"

#include "yuzu/Anf/Anf.h"
#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/Ops/Op.h"

#include <vector>

namespace yuzu::anf {
const Root *AnfLowerer::lowerRoot(const hir::Root *root) {
  std::vector<const Stmt *> stmts;
  for (const auto *stmt : root->getStmts()) {
    const auto *lowered = lowerStmt(stmt);
    // Flush any temporaries the statement produced, then the statement itself.
    stmts.insert(stmts.end(), intermediateStmts.begin(),
                 intermediateStmts.end());
    intermediateStmts.clear();
    stmts.push_back(lowered);
  }
  return ctx.build(root, &AnfBuilder::makeRoot, stmts);
}

const Ident *AnfLowerer::lowerIdent(const hir::Ident *ident) {
  return ctx.build(ident, &AnfBuilder::makeIdent, ident->getName());
}

const Param *AnfLowerer::lowerParam(const hir::Param *param) {
  const auto *ident = lowerIdent(param->getName());
  const auto *type = hirCtx.getTypeContext().typeOf(param);

  // Paramaters have no value by default, until default parameters are
  // available.
  const Expr *value = nullptr;

  const auto *binding =
      ctx.build(param, &AnfBuilder::makeBinding, ident, value, type);
  hirToAnfBindingMap[param] = binding;

  return ctx.build(param, &AnfBuilder::makeParam, binding);
}
} // namespace yuzu::anf