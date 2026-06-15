#include "yuzu/Hir/HirLowerer.h"

#include "yuzu/Ast/Ast.h"
#include "yuzu/Hir/Ops/Op.h"

#include <string>

namespace yuzu::hir {
const Expr *HirLowerer::lowerFromRel(ast::FromExpr expr) {
  const auto relation = expr.getRelation();
  if (!relation) {
    error(expr, "`from` is missing its relation").emit();
    return nullptr;
  }
  const Ident *loweredRelation = lowerIdent(*relation);
  if (!loweredRelation) {
    return nullptr;
  }

  // The alias is optional (`from t` vs `from t e`).
  const Ident *loweredAlias = nullptr;
  if (const auto alias = expr.getAlias()) {
    loweredAlias = lowerIdent(*alias);
  }

  const auto *hir = ctx.getBuilder().makeFromRel(loweredRelation, loweredAlias);
  ctx.getSourceTable().bind(hir->getId(), expr);
  return hir;
}

const SelectItem *HirLowerer::lowerSelectItem(ast::SelectItem item) {
  const auto astExpr = item.getExpr();
  if (!astExpr) {
    error(item, "select item is missing its expression").emit();
    return nullptr;
  }
  const Expr *loweredExpr = lowerExpr(*astExpr);
  if (!loweredExpr) {
    return nullptr;
  }

  const Ident *loweredAlias = nullptr;
  if (const auto alias = item.getAlias()) {
    loweredAlias = lowerIdent(*alias);
  }

  const auto *hir = ctx.getBuilder().makeSelectItem(loweredExpr, loweredAlias);
  ctx.getSourceTable().bind(hir->getId(), item);
  return hir;
}

const Expr *HirLowerer::lowerSelectRel(ast::SelectExpr expr) {
  const auto input = expr.getInput();
  if (!input) {
    error(expr, "`select` is missing its input relation").emit();
    return nullptr;
  }
  const Expr *loweredInput = lowerExpr(*input);
  if (!loweredInput) {
    return nullptr;
  }

  std::vector<const SelectItem *> items;
  for (const ast::SelectItem item : expr.getItems()) {
    if (const SelectItem *lowered = lowerSelectItem(item)) {
      items.push_back(lowered);
    }
  }

  const auto *hir = ctx.getBuilder().makeSelectRel(loweredInput, items);
  ctx.getSourceTable().bind(hir->getId(), expr);
  return hir;
}
} // namespace yuzu::hir