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
  }
}

const Rel *AnfLowerer::lowerFromRel(const hir::FromRel *fromRel) {
  const auto *relation = lowerIdent(fromRel->getRelation());
  const auto *alias =
      fromRel->getAlias() ? lowerIdent(fromRel->getAlias()) : nullptr;
  const auto *type = ctx.typeOf(fromRel);

  // Bind the row so columns resolve the alias the ordinary way: `e` ->
  // VarAtom(row), `e.salary` -> FieldAtom(VarAtom(row), salary). The row is an
  // input, so the binding has no defining value. HIR resolves the alias to
  // itself, so that ident is the bindingMap key.
  if (fromRel->getAlias() != nullptr) {
    const auto *rowType = ctx.typeOf(fromRel->getAlias());
    const Expr *noValue = nullptr;
    const auto *row = ctx.build(fromRel->getAlias(), &AnfBuilder::makeBinding,
                                alias, noValue, rowType);
    hirToAnfMap[fromRel->getAlias()] =
        ctx.build(fromRel->getAlias(), &AnfBuilder::makeVarAtom, row, rowType);
  }

  return ctx.build(fromRel, &AnfBuilder::makeFromRel, relation, alias, type);
}

const Rel *AnfLowerer::lowerSelectRel(const hir::SelectRel *selectRel) {
  // Lower the input first — a `from` input registers its row binding here, so
  // the columns below can resolve the alias.
  const auto *input = lowerExpr(selectRel->getInput());
  const auto *type = ctx.typeOf(selectRel);

  std::vector<const SelectItem *> items;
  items.reserve(selectRel->getItems().size());
  for (const auto *item : selectRel->getItems()) {
    items.push_back(lowerSelectItem(item));
  }

  bindColumns(selectRel);

  return ctx.build(selectRel, &AnfBuilder::makeSelectRel, input, items, type);
}

void AnfLowerer::bindColumns(const hir::SelectRel *selectRel) {
  // Mirror a `from`'s row binding for a `select`: synthesize a row for this
  // relation's output, and map each named column to a `FieldAtom` over it. A
  // later stage's reference to the column then lowers to that selection.
  const auto *relation = types::RelationType::cast(ctx.typeOf(selectRel));
  if (relation == nullptr) {
    return;
  }
  const auto *rowType = relation->getElement();
  const Expr *noValue = nullptr;
  const auto *row = ctx.build(selectRel, &AnfBuilder::makeBinding,
                              ctx.makeTemp(), noValue, rowType);

  for (const auto *item : selectRel->getItems()) {
    if (item->getExpr() == nullptr) {
      continue;
    }
    // The HIR ident a downstream reference resolves to: the `as` alias, or the
    // column's own identifier when it's a bare name. An expression column with
    // no alias is anonymous — it has no name to reference.
    const hir::Ident *name = item->getAlias();
    if (name == nullptr) {
      if (const auto *ident = hir::IdentExpr::cast(item->getExpr())) {
        name = ident->getName();
      }
    }
    if (name == nullptr) {
      continue;
    }

    const auto *columnType = ctx.typeOf(item->getExpr());
    const auto *rowAtom =
        ctx.build(name, &AnfBuilder::makeVarAtom, row, rowType);
    const auto *field = ctx.build(name, &AnfBuilder::makeFieldAtom, rowAtom,
                                  lowerIdent(name), columnType);
    hirToAnfMap[name] = field;
  }
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

} // namespace yuzu::anf
