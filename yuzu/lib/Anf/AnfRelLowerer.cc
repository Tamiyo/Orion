#include "yuzu/Anf/AnfLowerer.h"

#include "yuzu/Anf/Anf.h"
#include "yuzu/Hir/Hir.h"

#include <utility>
#include <vector>

namespace yuzu::anf {

const Rel *AnfLowerer::lowerRel(const hir::Rel *rel) {
  switch (rel->getRelKind()) {
  case hir::RelKind::FromRel:
    return lowerFromRel(hir::FromRel::cast(rel));
  case hir::RelKind::SelectRel:
    return lowerSelectRel(hir::SelectRel::cast(rel));
  }
}

const Rel *AnfLowerer::lowerFromRel(const hir::FromRel *fromRel) {
  const auto *relation = lowerIdent(fromRel->getRelation());
  const auto *alias =
      fromRel->getAlias() ? lowerIdent(fromRel->getAlias()) : nullptr;
  const auto *type = hirCtx.getTypeContext().typeOf(fromRel);

  // Bind the row so columns resolve the alias the ordinary way: `e` ->
  // VarAtom(row), `e.salary` -> FieldAtom(VarAtom(row), salary). The row is an
  // input, so the binding has no defining value. HIR resolves the alias to
  // itself, so that ident is the bindingMap key.
  if (fromRel->getAlias() != nullptr) {
    const auto *rowType = hirCtx.getTypeContext().typeOf(fromRel->getAlias());
    const Expr *noValue = nullptr;
    const auto *row = ctx.build(fromRel->getAlias(), &AnfBuilder::makeBinding,
                                alias, noValue, rowType);
    hirToAnfBindingMap[fromRel->getAlias()] = row;
  }

  return ctx.build(fromRel, &AnfBuilder::makeFromRel, relation, alias, type);
}

const Rel *AnfLowerer::lowerSelectRel(const hir::SelectRel *selectRel) {
  // Lower the input first — a `from` input registers its row binding here, so
  // the columns below can resolve the alias.
  const auto *input = lowerExpr(selectRel->getInput());
  const auto *type = hirCtx.getTypeContext().typeOf(selectRel);

  std::vector<const SelectItem *> items;
  items.reserve(selectRel->getItems().size());
  for (const auto *item : selectRel->getItems()) {
    items.push_back(lowerSelectItem(item));
  }

  return ctx.build(selectRel, &AnfBuilder::makeSelectRel, input, items, type);
}

const SelectItem *
AnfLowerer::lowerSelectItem(const hir::SelectItem *selectItem) {
  // Each column is its own per-row block: temporaries land in a fresh buffer,
  // and the block resolves to the column value at its tail.
  std::vector<const Stmt *> saved = std::move(intermediateStmts);
  intermediateStmts.clear();

  const auto *value = lowerExpr(selectItem->getExpr());
  const auto *tail = ctx.build(selectItem, &AnfBuilder::makeExprStmt, value);

  std::vector<const Stmt *> stmts = std::move(intermediateStmts);
  stmts.push_back(tail);
  const auto *body = ctx.build(selectItem, &AnfBuilder::makeBlockStmt, stmts);

  intermediateStmts = std::move(saved);

  const auto *alias =
      selectItem->getAlias() ? lowerIdent(selectItem->getAlias()) : nullptr;
  return ctx.build(selectItem, &AnfBuilder::makeSelectItem, body, alias);
}

} // namespace yuzu::anf
