#include "yuzu/Hir/HirLowerer.h"

#include "yuzu/Ast/Ast.h"

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

const Expr *HirLowerer::lowerWhereRel(ast::WhereExpr expr) {
  const auto input = expr.getInput();
  if (!input) {
    error(expr, "`where` is missing its input relation").emit();
    return nullptr;
  }
  const Expr *loweredInput = lowerExpr(*input);
  if (!loweredInput) {
    return nullptr;
  }

  const auto predicate = expr.getPredicate();
  if (!predicate) {
    error(expr, "`where` is missing its predicate").emit();
    return nullptr;
  }
  const Expr *loweredPredicate = lowerExpr(*predicate);
  if (!loweredPredicate) {
    return nullptr;
  }

  const auto *hir =
      ctx.getBuilder().makeWhereRel(loweredInput, loweredPredicate);
  ctx.getSourceTable().bind(hir->getId(), expr);
  return hir;
}

const Expr *HirLowerer::lowerDistinctRel(ast::DistinctExpr expr) {
  const auto input = expr.getInput();
  if (!input) {
    error(expr, "`distinct` is missing its input relation").emit();
    return nullptr;
  }
  const Expr *loweredInput = lowerExpr(*input);
  if (!loweredInput) {
    return nullptr;
  }

  const auto *hir = ctx.getBuilder().makeDistinctRel(loweredInput);
  ctx.getSourceTable().bind(hir->getId(), expr);
  return hir;
}

const Expr *HirLowerer::lowerDropRel(ast::DropExpr expr) {
  const auto input = expr.getInput();
  if (!input) {
    error(expr, "`drop` is missing its input relation").emit();
    return nullptr;
  }
  const Expr *loweredInput = lowerExpr(*input);
  if (!loweredInput) {
    return nullptr;
  }

  std::vector<const Ident *> columns;
  for (const ast::Ident column : expr.getColumns()) {
    if (const Ident *lowered = lowerIdent(column)) {
      columns.push_back(lowered);
    }
  }

  const auto *hir = ctx.getBuilder().makeDropRel(loweredInput, columns);
  ctx.getSourceTable().bind(hir->getId(), expr);
  return hir;
}

const RenameItem *HirLowerer::lowerRenameItem(ast::RenameItem item) {
  const auto from = item.getFrom();
  const auto to = item.getTo();
  if (!from || !to) {
    error(item, "rename item is missing its `from` or `to` name").emit();
    return nullptr;
  }
  const Ident *loweredFrom = lowerIdent(*from);
  const Ident *loweredTo = lowerIdent(*to);
  if (!loweredFrom || !loweredTo) {
    return nullptr;
  }

  const auto *hir = ctx.getBuilder().makeRenameItem(loweredFrom, loweredTo);
  ctx.getSourceTable().bind(hir->getId(), item);
  return hir;
}

const Expr *HirLowerer::lowerRenameRel(ast::RenameExpr expr) {
  const auto input = expr.getInput();
  if (!input) {
    error(expr, "`rename` is missing its input relation").emit();
    return nullptr;
  }
  const Expr *loweredInput = lowerExpr(*input);
  if (!loweredInput) {
    return nullptr;
  }

  std::vector<const RenameItem *> items;
  for (const ast::RenameItem item : expr.getItems()) {
    if (const RenameItem *lowered = lowerRenameItem(item)) {
      items.push_back(lowered);
    }
  }

  const auto *hir = ctx.getBuilder().makeRenameRel(loweredInput, items);
  ctx.getSourceTable().bind(hir->getId(), expr);
  return hir;
}

const Expr *HirLowerer::lowerExtendRel(ast::ExtendExpr expr) {
  const auto input = expr.getInput();
  if (!input) {
    error(expr, "`extend` is missing its input relation").emit();
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

  const auto *hir = ctx.getBuilder().makeExtendRel(loweredInput, items);
  ctx.getSourceTable().bind(hir->getId(), expr);
  return hir;
}
} // namespace yuzu::hir