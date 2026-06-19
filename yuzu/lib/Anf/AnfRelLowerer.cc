#include "yuzu/Anf/AnfLowerer.h"

#include "yuzu/Anf/Anf.h"
#include "yuzu/Hir/Hir.h"
#include "yuzu/Types/Type.h"

#include <utility>
#include <vector>

namespace yuzu::anf {

const Rel *AnfLowerer::lowerRel(const hir::Rel *rel) {
  switch (rel->getRelKind()) {
  case hir::RelKind::FromRel:
    return lowerFromRel(hir::FromRel::cast(rel));
  case hir::RelKind::SelectRel:
    return lowerSelectRel(hir::SelectRel::cast(rel));
  case hir::RelKind::WhereRel:
    return lowerWhereRel(hir::WhereRel::cast(rel));
  case hir::RelKind::DistinctRel:
    return lowerDistinctRel(hir::DistinctRel::cast(rel));
  case hir::RelKind::DropRel:
    return lowerDropRel(hir::DropRel::cast(rel));
  }
}

const Atom *AnfLowerer::makeRowValue(const types::Type *rowType,
                                     const hir::HirNode *origin) {
  const Expr *noValue = nullptr;
  const auto *binding = ctx.build(origin, &AnfBuilder::makeBinding,
                                  ctx.makeTemp(), noValue, rowType);
  return ctx.build(origin, &AnfBuilder::makeVarAtom, binding, rowType);
}

const Rel *AnfLowerer::lowerFromRel(const hir::FromRel *fromRel) {
  const auto *relation = lowerIdent(fromRel->getRelation());
  const auto *alias =
      fromRel->getAlias() ? lowerIdent(fromRel->getAlias()) : nullptr;
  const auto *type = ctx.typeOf(fromRel);

  // The row value the next stage's columns select from. An explicit alias is
  // also bound (so `e.salary` resolves the ordinary way); a bare `salary`
  // resolves against `currentRow` directly.
  const auto *rowType = types::RelationType::cast(type)->getElement();
  const auto *row = makeRowValue(rowType, fromRel);
  if (fromRel->getAlias() != nullptr) {
    hirToAnfMap[fromRel->getAlias()] = row;
  }
  currentRow = row;

  return ctx.build(fromRel, &AnfBuilder::makeFromRel, relation, alias, type);
}

const Rel *AnfLowerer::lowerSelectRel(const hir::SelectRel *selectRel) {
  // Lower the input first; it sets `currentRow` to its output row, which the
  // items below select their columns from.
  const auto *input = lowerExpr(selectRel->getInput());
  const auto *type = ctx.typeOf(selectRel);

  std::vector<const SelectItem *> items;
  items.reserve(selectRel->getItems().size());
  for (const auto *item : selectRel->getItems()) {
    items.push_back(lowerSelectItem(item));
  }

  // Hand the next stage this select's output row.
  currentRow =
      makeRowValue(types::RelationType::cast(type)->getElement(), selectRel);

  return ctx.build(selectRel, &AnfBuilder::makeSelectRel, input, items, type);
}

const Thunk *AnfLowerer::lowerThunk(const hir::Expr *expr,
                                    const hir::HirNode *origin) {
  // A per-row block: temporaries land in a fresh buffer, and the block resolves
  // to the value at its tail. The caller's buffer is saved and restored.
  std::vector<const Stmt *> saved = std::move(intermediateStmts);
  intermediateStmts.clear();

  const auto *value = lowerExpr(expr);
  const auto *tail = ctx.build(origin, &AnfBuilder::makeExprStmt, value);

  std::vector<const Stmt *> stmts = std::move(intermediateStmts);
  stmts.push_back(tail);
  const auto *body = ctx.build(origin, &AnfBuilder::makeThunk, stmts);

  intermediateStmts = std::move(saved);
  return body;
}

const SelectItem *
AnfLowerer::lowerSelectItem(const hir::SelectItem *selectItem) {
  const auto *body = lowerThunk(selectItem->getExpr(), selectItem);
  const auto *alias =
      selectItem->getAlias() ? lowerIdent(selectItem->getAlias()) : nullptr;
  return ctx.build(selectItem, &AnfBuilder::makeSelectItem, body, alias);
}

const Rel *AnfLowerer::lowerWhereRel(const hir::WhereRel *whereRel) {
  // Lower the input first (a `from` input registers its row binding so the
  // predicate can resolve the alias). `where` is pass-through: the input's
  // column bindings remain valid for the next stage, so no rebinding here.
  const auto *input = lowerExpr(whereRel->getInput());
  const auto *type = ctx.typeOf(whereRel);
  const auto *predicate = lowerThunk(whereRel->getPredicate(), whereRel);

  return ctx.build(whereRel, &AnfBuilder::makeWhereRel, input, predicate, type);
}

const Rel *AnfLowerer::lowerDistinctRel(const hir::DistinctRel *distinctRel) {
  // `distinct` is pass-through: the input's column bindings stay valid (dedup
  // preserves the row positionally), so no rebinding here.
  const auto *input = lowerExpr(distinctRel->getInput());
  const auto *type = ctx.typeOf(distinctRel);
  return ctx.build(distinctRel, &AnfBuilder::makeDistinctRel, input, type);
}

const Rel *AnfLowerer::lowerDropRel(const hir::DropRel *dropRel) {
  const auto *input = lowerExpr(dropRel->getInput());
  const auto *type = ctx.typeOf(dropRel);

  std::vector<const Ident *> columns;
  columns.reserve(dropRel->getColumns().size());
  for (const auto *column : dropRel->getColumns()) {
    columns.push_back(lowerIdent(column));
  }

  // Drop reshapes the row (positions shift), so the next stage selects from
  // this op's output row, not the input's.
  currentRow =
      makeRowValue(types::RelationType::cast(type)->getElement(), dropRel);

  return ctx.build(dropRel, &AnfBuilder::makeDropRel, input, columns, type);
}

} // namespace yuzu::anf
