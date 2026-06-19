#include "yuzu/Hir/Types/TypeInferrer.h"

#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/HirContext.h"
#include "yuzu/Hir/Ops/OpResolve.h"
#include "yuzu/Hir/Resolve/HirSymbolTable.h"
#include "yuzu/Hir/Types/TypeCoercion.h"
#include "yuzu/Types/Type.h"
#include "yuzu/Util/Unicode.h"

#include <llvm/ADT/STLExtras.h>
#include <llvm/Support/FormatVariadic.h>

#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace yuzu::hir {
using namespace yuzu::types;

namespace {
// A relation's element is always its row struct.
const StructType *rowOf(const Type *relationType) {
  return StructType::cast(RelationType::cast(relationType)->getElement());
}
} // namespace

void TypeInferrer::visitBoolLit(const BoolLit *n) {
  auto &types = ctx.getTypeContext();
  auto &typeFactory = types.getTypeFactory();
  types.bind(n, typeFactory.getBoolType());
}

void TypeInferrer::visitStringLit(const StringLit *n) {
  auto &types = ctx.getTypeContext();
  auto &typeFactory = types.getTypeFactory();
  types.bind(n, typeFactory.getStrType());
}

// Untyped numeric literals are holes; context pins them, else they default.
void TypeInferrer::visitIntLit(const IntLit *n) {
  auto &types = ctx.getTypeContext();
  types.bind(n, types.makeTypeHole(InferKind::Int));
}

void TypeInferrer::visitFloatLit(const FloatLit *n) {
  auto &types = ctx.getTypeContext();
  types.bind(n, types.makeTypeHole(InferKind::Float));
}

void TypeInferrer::visitIdentExpr(const IdentExpr *n) {
  auto &types = ctx.getTypeContext();
  auto &typeFactory = types.getTypeFactory();

  // A bare name that names a column of the current pipe stage's input row
  // resolves to that column, shadowing outer bindings (SQL-style). No
  // resolution is recorded — ANF re-derives it as a field of the current row.
  if (currentRow != nullptr) {
    if (const Type *fieldType =
            currentRow->findField(n->getName()->getName())) {
      types.bind(n, fieldType);
      return;
    }
  }

  const auto lookupResult = ctx.getSymbolTable().lookup(n->getName());

  if (!lookupResult) {
    // A table is reachable only from a query's `from`, never as a host value.
    if (ctx.getSymbolTable().lookupTable(n->getName()->getName())) {
      ctx.error(n, llvm::formatv("`{0}` is a table; query it with `from`",
                                 util::toUtf8(n->getName()->getName()))
                       .str())
          .emit();
    } else {
      ctx.error(n, "unresolved identifier").emit();
    }
    types.bind(n, typeFactory.getErrorType());
    return;
  }

  // Record which declaration this name binds to, so ANF lowering (and any later
  // pass) gets the resolution without redoing the scope walk.
  ctx.getResolutions().bind(n->getName()->getId(), *lookupResult);

  // A function reference resolves its signature lazily on first use (forward
  // references); other bindings already carry a type in the side table.
  const Type *type = std::visit(
      [&](const auto *decl) -> const Type * {
        if constexpr (std::is_same_v<std::decay_t<decltype(*decl)>, FuncStmt>) {
          return signatureOf(decl);
        } else {
          return types.typeOf(decl);
        }
      },
      *lookupResult);

  types.bind(n, type);
}

void TypeInferrer::visitCallExpr(const CallExpr *callExpr) {
  auto &types = ctx.getTypeContext();

  // A failed operand poisons the call silently — no cascading diagnostic.
  for (const Expr *arg : callExpr->getArgs()) {
    const Type *type = types.typeOf(arg);
    if (type->getKind() == TypeKind::Error) {
      types.bind(callExpr, types.getTypeFactory().getErrorType());
      return;
    }
  }

  const Type *resolvedType =
      resolve(callExpr->getOp(), callExpr->getArgs(), ctx);

  types.bind(callExpr, resolvedType);
}

void TypeInferrer::visitFuncCallExpr(const FuncCallExpr *funcCallExpr) {
  auto &types = ctx.getTypeContext();
  const Expr *callee = funcCallExpr->getCallee();
  const Type *calleeType = types.typeOf(callee);

  // A failed callee already reported; early exist.
  if (calleeType->getKind() == TypeKind::Error) {
    types.bind(funcCallExpr, types.getTypeFactory().getErrorType());
    return;
  }

  const auto *funcType = FuncType::cast(calleeType);
  if (!funcType) {
    ctx.error(callee, llvm::formatv("`{0}` is not callable",
                                    asString(calleeType->getKind()))
                          .str())
        .emit();
    types.bind(funcCallExpr, types.getTypeFactory().getErrorType());
    return;
  }

  // Arity check.
  const auto args = funcCallExpr->getArgs();
  const auto argTypes = funcType->getArgTypes();
  if (args.size() != argTypes.size()) {
    ctx.error(funcCallExpr, llvm::formatv("expected {0} argument(s), found {1}",
                                          argTypes.size(), args.size())
                                .str())
        .emit();
    types.bind(funcCallExpr, funcType->getReturnType());
    return;
  }

  // Per-call substitution of `[T]` markers → fresh holes, so calls don't
  // disturb the stored template.
  llvm::DenseMap<const Type *, const Type *> subst;

  // Each argument must be assignable to its instantiated parameter.
  for (size_t i = 0; i < args.size(); ++i) {
    const Type *param = substituteType(argTypes[i], subst);
    if (!isAssignable(args[i], param)) {
      ctx.error(args[i],
                llvm::formatv("argument of type `{0}` is not assignable to "
                              "parameter of type `{1}`",
                              asString(types.typeOf(args[i])->getKind()),
                              asString(types.resolveType(param)->getKind()))
                    .str())
          .emit();
    }
  }

  // Discharge `where` bounds: the type each marker was instantiated to must
  // implement the traits the bound promised.
  auto &registry = types.getTraitRegistry();
  for (const auto &[markerType, holeType] : subst) {
    const auto it = markerBounds.find(TypeParamType::cast(markerType));
    if (it == markerBounds.end()) {
      continue;
    }
    const Type *concrete = types.resolveType(holeType);
    for (const std::u32string_view trait : it->second) {
      if (!registry.lookupImpl(trait, concrete)) {
        ctx.error(funcCallExpr, llvm::formatv("`{0}` does not implement `{1}`",
                                              asString(concrete->getKind()),
                                              util::toUtf8(trait))
                                    .str())
            .emit();
      }
    }
  }

  // Leave the return as a (possibly open) hole so an annotation can pin it.
  types.bind(funcCallExpr, substituteType(funcType->getReturnType(), subst));
}

const Type *TypeInferrer::substituteType(
    const Type *type, llvm::DenseMap<const Type *, const Type *> &subst) {
  auto &types = ctx.getTypeContext();
  auto &typeFactory = types.getTypeFactory();

  if (TypeParamType::cast(type)) {
    // Same marker → same fresh hole within this call. Keyed by marker identity
    // (interned per declaration), so a captured `[T]` and an own `[U]` stay
    // distinct even at the same position.
    auto [it, inserted] = subst.try_emplace(type, nullptr);
    if (inserted) {
      it->second = types.makeTypeHole(InferKind::General);
    }
    return it->second;
  }

  // Nested function types substitute component-wise; concretes pass through.
  if (const auto *funcType = FuncType::cast(type)) {
    std::vector<const Type *> params;
    params.reserve(funcType->getArgTypes().size());
    for (const Type *argType : funcType->getArgTypes()) {
      params.push_back(substituteType(argType, subst));
    }

    const Type *returnType = substituteType(funcType->getReturnType(), subst);
    return typeFactory.getFuncTy(params, returnType);
  }

  return type;
}

bool TypeInferrer::isAssignable(const Expr *value, const Type *target) {
  auto &types = ctx.getTypeContext();
  if (types.unifyTypes(types.typeOf(value), target)) {
    return true;
  }
  types.concretize(value);
  return coercesTo(value, types.resolveType(target), ctx);
}

void TypeInferrer::visitLetStmt(const LetStmt *n) {
  auto &types = ctx.getTypeContext();

  const Expr *expr = n->getExpr();

  if (const Type *type = resolveTypeAnnotation(n->getTypeAnnotation(), n)) {
    types.bind(n, type);

    // Check if the type annotation is assignable. Skip error types because they
    // are inheriently not assignable.
    if (type->getKind() != TypeKind::Error && !isAssignable(expr, type)) {
      ctx.error(n, llvm::formatv("value of type `{0}` is not assignable to "
                                 "`{1}`",
                                 asString(types.typeOf(expr)->getKind()),
                                 asString(type->getKind()))
                       .str())
          .emit();
    }
  }
  // No annotation: keep the initializer's open type so a later use can pin
  // it.
  else {
    types.bind(n, types.typeOf(expr));
  }

  ctx.getSymbolTable().bind(n->getName(), n);
}

void TypeInferrer::visitAssignStmt(const AssignStmt *n) {
  auto &types = ctx.getTypeContext();
  const Expr *target = n->getTarget();
  const Expr *value = n->getValue();

  // A poisoned target/value was already reported; don't cascade.
  if (types.typeOf(target)->getKind() == TypeKind::Error ||
      types.typeOf(value)->getKind() == TypeKind::Error) {
    return;
  }

  // The target must be an assignable place: an identifier bound by `let mut`.
  const auto *ident = IdentExpr::cast(target);
  if (!ident) {
    ctx.error(target, "cannot assign to this expression").emit();
    return;
  }

  const auto binding = ctx.getSymbolTable().lookup(ident->getName());
  const LetStmt *const *letBinding =
      binding ? std::get_if<const LetStmt *>(&*binding) : nullptr;
  if (!letBinding) {
    ctx.error(target,
              llvm::formatv("cannot assign to `{0}`, which is not a variable",
                            util::toUtf8(ident->getName()->getName()))
                  .str())
        .emit();
    return;
  }

  if ((*letBinding)->getMutability() != Mutability::Mutable) {
    ctx.error(target,
              llvm::formatv("cannot assign to immutable binding `{0}`; declare "
                            "it with `let mut`",
                            util::toUtf8(ident->getName()->getName()))
                  .str())
        .emit();
    return;
  }

  // The value must be assignable to the binding's type.
  if (!isAssignable(value, types.typeOf(target))) {
    ctx.error(n, llvm::formatv("value of type `{0}` is not assignable to `{1}`",
                               asString(types.typeOf(value)->getKind()),
                               asString(types.typeOf(target)->getKind()))
                     .str())
        .emit();
  }
}

void TypeInferrer::traverseRoot(const Root *n) { hoistAndWalk(n->getStmts()); }

void TypeInferrer::traverseBlockStmt(const BlockStmt *n) {
  hoistAndWalk(n->getStmts());
  visitBlockStmt(n);
}

void TypeInferrer::hoistAndWalk(llvm::ArrayRef<const Stmt *> stmts) {
  auto &symbols = ctx.getSymbolTable();

  // Pass 1a: register structs as named types — before tables/functions that may
  // reference them. (Cross-struct forward references are resolved in order.)
  for (const Stmt *stmt : stmts) {
    if (const auto *structStmt = StructStmt::cast(stmt)) {
      registerStruct(structStmt);
    }
  }

  // Pass 1b: register tables (rows reference the structs above).
  for (const Stmt *stmt : stmts) {
    if (const auto *tableStmt = TableStmt::cast(stmt)) {
      registerTable(tableStmt);
    }
  }

  // Pass 1c: bind every function's name so a body can reference siblings
  // declared later in the same scope. Signatures resolve lazily on first use
  // (`signatureOf`), so declaration order doesn't matter.
  for (const Stmt *stmt : stmts) {
    if (const auto *funcStmt = FuncStmt::cast(stmt)) {
      symbols.bind(funcStmt->getName(), funcStmt);
    }
  }

  // Pass 2: type each statement.
  for (const Stmt *stmt : stmts) {
    visit(stmt);
  }
}

void TypeInferrer::registerStruct(const StructStmt *n) {
  auto &types = ctx.getTypeContext();
  auto &typeFactory = types.getTypeFactory();
  auto &symbols = ctx.getSymbolTable();

  std::vector<StructField> fields;
  fields.reserve(n->getFields().size());
  for (const StructFieldDecl *field : n->getFields()) {
    const Type *fieldType = resolveTypeAnnotation(field->getType(), field);
    fields.push_back(StructField{field->getName()->getName(), fieldType});
  }

  const StructType *structType =
      typeFactory.getStructType(n->getName()->getName(), fields);
  symbols.bindType(n->getName()->getName(), structType);
  types.bind(n, structType);
}

void TypeInferrer::registerTable(const TableStmt *n) {
  auto &types = ctx.getTypeContext();
  auto &typeFactory = types.getTypeFactory();
  auto &symbols = ctx.getSymbolTable();

  const StructType *row = nullptr;
  if (const Ident *rowStruct = n->getRowStruct()) {
    // Named form: the rows are a declared struct.
    const Type *named = symbols.lookupType(rowStruct->getName());
    row = named ? StructType::cast(named) : nullptr;
    if (!row) {
      ctx.error(n, llvm::formatv("`{0}` is not a struct",
                                 util::toUtf8(rowStruct->getName()))
                       .str())
          .emit();
      return;
    }
  } else {
    // Inline form: an anonymous struct named after the table.
    std::vector<StructField> fields;
    fields.reserve(n->getInlineFields().size());
    for (const StructFieldDecl *field : n->getInlineFields()) {
      const Type *fieldType = resolveTypeAnnotation(field->getType(), field);
      fields.push_back(StructField{field->getName()->getName(), fieldType});
    }
    row = typeFactory.getStructType(n->getName()->getName(), fields);
  }

  const RelationType *relation = typeFactory.getRelationType(row);
  symbols.bindTable(n->getName()->getName(), relation);
  types.bind(n, relation);
}

const FuncType *TypeInferrer::signatureOf(const FuncStmt *n) {
  auto &types = ctx.getTypeContext();

  // Memoized: resolve the signature once, then serve it from the side table.
  if (const Type *cached = types.typeOf(n)) {
    return FuncType::cast(cached);
  }

  // Resolve in an isolated scope so the function's params/type-params don't
  // leak into the requesting scope. (Resolution sees the request site's
  // visible types, which is fine while type lookup only reaches type-params
  // and global builtins; a nested capture would need declaration-site scope.)
  const HirScopeGuard guard =
      ctx.getSymbolTable().pushScope(HirScopeKind::Func);

  return resolveFuncType(n);
}

const TypeParamType *TypeInferrer::markerFor(const Ident *decl,
                                             uint32_t index) {
  auto [it, inserted] = typeParamMarkers.try_emplace(decl, nullptr);
  if (inserted) {
    it->second = ctx.getTypeContext().getTypeFactory().getTypeParamType(
        index, decl->getName());
  }
  return it->second;
}

void TypeInferrer::resolveTraitBounds(const FuncStmt *funcStmt) {
  auto &symbols = ctx.getSymbolTable();
  auto &registry = ctx.getTypeContext().getTraitRegistry();

  for (const TypeBound *bound : funcStmt->getBounds()) {
    const std::u32string_view subject = bound->getSubject()->getName();
    const auto *marker = TypeParamType::cast(symbols.lookupType(subject));
    if (!marker) {
      ctx.error(bound, llvm::formatv("`{0}` is not a type parameter of this "
                                     "function",
                                     util::toUtf8(subject))
                           .str())
          .emit();
      continue;
    }

    for (const TraitRef *traitRef : bound->getTraits()) {
      const std::u32string_view trait = traitRef->getName()->getName();
      if (!registry.isRegistered(trait)) {
        ctx.error(
               traitRef,
               llvm::formatv("unknown trait `{0}`", util::toUtf8(trait)).str())
            .emit();
        continue;
      }
      registry.registerImpl(trait, marker);
      markerBounds[marker].push_back(trait);
    }
  }
}

const FuncType *TypeInferrer::resolveFuncType(const FuncStmt *n) {
  auto &types = ctx.getTypeContext();
  auto &typeFactory = types.getTypeFactory();
  auto &symbols = ctx.getSymbolTable();

  // Type parameters (generics) — bound in the function scope. Markers are
  // interned by declaration, so the body sees the same ones as the signature.
  for (size_t i = 0; i < n->getTypeParams().size(); ++i) {
    const Ident *typeParam = n->getTypeParams()[i];
    symbols.bindType(typeParam->getName(),
                     markerFor(typeParam, static_cast<uint32_t>(i)));
  }

  // `where` bounds — record each named trait as implemented by the marker, so
  // an operator on a bound `T` resolves; the call site later checks the
  // instantiating type really implements it.
  resolveTraitBounds(n);

  // Parameters — resolve each annotation, binding its type and name.
  std::vector<const Type *> paramTypes;
  paramTypes.reserve(n->getParams().size());
  for (const Param *param : n->getParams()) {
    // SAFETY: TypeAnnotations on function parameters are never null.
    const TypeAnnotation *annotation = param->getTypeAnnotation();
    const Type *paramType = resolveTypeAnnotation(annotation, param);

    types.bind(param, paramType);
    symbols.bind(param->getName(), param);
    paramTypes.push_back(paramType);
  }

  // Return type — an omitted annotation defaults to `unit`.
  // SAFETY: `resolveTypeAnnotation` never returns null for a present
  // annotation.
  const Type *returnType =
      n->getReturnTypeAnnotation()
          ? resolveTypeAnnotation(n->getReturnTypeAnnotation(), n)
          : typeFactory.getUnitType();

  // The signature; a template when generic (`[T]` params are markers).
  const FuncType *funcType = typeFactory.getFuncTy(paramTypes, returnType);
  types.bind(n, funcType);
  return funcType;
}

void TypeInferrer::traverseFuncStmt(const FuncStmt *n) {
  auto &types = ctx.getTypeContext();
  auto &typeFactory = types.getTypeFactory();
  auto &symbols = ctx.getSymbolTable();

  // The signature is resolved once (here if not already, else served from the
  // cache); the body references it rather than re-resolving.
  const FuncType *funcType = signatureOf(n);

  // Fresh scope for the body. Re-establish the function's bindings: type-param
  // markers (interned by declaration, so identical to the cached signature's)
  // and parameter values (their types are already in the side table).
  const HirScopeGuard guard = symbols.pushScope(HirScopeKind::Func);
  for (size_t i = 0; i < n->getTypeParams().size(); ++i) {
    const Ident *typeParam = n->getTypeParams()[i];
    symbols.bindType(typeParam->getName(),
                     markerFor(typeParam, static_cast<uint32_t>(i)));
  }

  for (const Param *param : n->getParams()) {
    symbols.bind(param->getName(), param);
  }

  const Type *prevExpectedReturn = expectedReturn;
  expectedReturn =
      funcType ? funcType->getReturnType() : typeFactory.getErrorType();
  visit(n->getBody());
  expectedReturn = prevExpectedReturn;
}

// A query root pushes one row scope that spans every stage, then types the
// query bottom-up via `inferQuery` (manual recursion, so the `from` alias
// stays visible through the `select`).
void TypeInferrer::traverseFromRel(const FromRel *n) {
  const HirScopeGuard guard =
      ctx.getSymbolTable().pushScope(HirScopeKind::Block);
  inferQuery(n);
}
void TypeInferrer::traverseSelectRel(const SelectRel *n) {
  const HirScopeGuard guard =
      ctx.getSymbolTable().pushScope(HirScopeKind::Block);
  inferQuery(n);
}
void TypeInferrer::traverseWhereRel(const WhereRel *n) {
  const HirScopeGuard guard =
      ctx.getSymbolTable().pushScope(HirScopeKind::Block);
  inferQuery(n);
}
void TypeInferrer::traverseDistinctRel(const DistinctRel *n) {
  const HirScopeGuard guard =
      ctx.getSymbolTable().pushScope(HirScopeKind::Block);
  inferQuery(n);
}
void TypeInferrer::traverseDropRel(const DropRel *n) {
  const HirScopeGuard guard =
      ctx.getSymbolTable().pushScope(HirScopeKind::Block);
  inferQuery(n);
}
void TypeInferrer::traverseRenameRel(const RenameRel *n) {
  const HirScopeGuard guard =
      ctx.getSymbolTable().pushScope(HirScopeKind::Block);
  inferQuery(n);
}

const Type *TypeInferrer::inferQuery(const Expr *query) {
  if (const auto *from = FromRel::cast(query)) {
    return inferFromRel(from);
  }
  if (const auto *select = SelectRel::cast(query)) {
    return inferSelectRel(select);
  }
  if (const auto *where = WhereRel::cast(query)) {
    return inferWhereRel(where);
  }
  if (const auto *distinct = DistinctRel::cast(query)) {
    return inferDistinctRel(distinct);
  }
  if (const auto *drop = DropRel::cast(query)) {
    return inferDropRel(drop);
  }
  if (const auto *rename = RenameRel::cast(query)) {
    return inferRenameRel(rename);
  }
  // The parser only ever builds query rels in query position.
  const Type *error = ctx.getTypeContext().getTypeFactory().getErrorType();
  ctx.getTypeContext().bind(query, error);
  return error;
}

const Type *TypeInferrer::inferFromRel(const FromRel *n) {
  auto &types = ctx.getTypeContext();
  auto &typeFactory = types.getTypeFactory();
  auto &symbols = ctx.getSymbolTable();

  const Ident *relation = n->getRelation();
  const RelationType *rel =
      relation ? symbols.lookupTable(relation->getName()) : nullptr;
  if (!rel) {
    ctx.error(n, llvm::formatv("`{0}` is not a table",
                               relation ? util::toUtf8(relation->getName())
                                        : std::string("?"))
                     .str())
        .emit();
    types.bind(n, typeFactory.getErrorType());
    return typeFactory.getErrorType();
  }

  // Bind the row alias to the relation's element type so later stages can
  // reference its columns. It resolves through the value namespace's
  // `typeOf(decl)` path (the alias `Ident` is the binding).
  if (const Ident *alias = n->getAlias()) {
    types.bind(alias, rel->getElement());
    symbols.bind(alias, alias);
  }

  types.bind(n, rel);
  return rel;
}

const Type *TypeInferrer::inferSelectRel(const SelectRel *n) {
  auto &types = ctx.getTypeContext();
  auto &typeFactory = types.getTypeFactory();
  auto &symbols = ctx.getSymbolTable();

  std::vector<StructField> outFields;
  outFields.reserve(n->getItems().size());
  uint32_t anonymous = 0;

  // Per pipe semantics, this stage sees only its immediate input. Type the
  // input in a child scope it populates (a `from` binds its row alias), then
  // type the items with that input's row as the current row, so bare column
  // names resolve against it. Dropping the scope keeps the alias from leaking
  // to the next stage.
  {
    const HirScopeGuard guard = symbols.pushScope(HirScopeKind::Block);
    const Type *inputType =
        n->getInput() ? inferQuery(n->getInput()) : typeFactory.getErrorType();
    if (inputType->getKind() == TypeKind::Error) {
      types.bind(n, typeFactory.getErrorType());
      return typeFactory.getErrorType();
    }

    const StructType *savedRow = currentRow;
    currentRow = rowOf(inputType);

    for (const SelectItem *item : n->getItems()) {
      const Expr *itemExpr = item->getExpr();
      if (itemExpr == nullptr) {
        continue;
      }
      visit(itemExpr); // types `e.id` / bare columns against the input row
      const Type *colType = types.typeOf(itemExpr);

      // Output column name: the `as` alias, else a bare identifier's own name,
      // else a field access's field, else a generated `%gN` for an anonymous
      // expression column.
      std::u32string_view colName;
      if (const Ident *alias = item->getAlias()) {
        colName = alias->getName();
      } else if (const auto *id = IdentExpr::cast(itemExpr)) {
        colName = id->getName()->getName();
      } else if (const auto *fa = FieldAccessExpr::cast(itemExpr)) {
        colName = fa->getField()->getName();
      } else {
        std::u32string generated = U"%g";
        for (char c : std::to_string(anonymous++)) {
          generated.push_back(static_cast<char32_t>(c));
        }
        colName = ctx.getStringInterner().intern(generated);
      }

      outFields.push_back(StructField{colName, colType});
    }

    currentRow = savedRow;
  }

  const StructType *outRow = typeFactory.getStructType(U"", outFields);
  const RelationType *outRel = typeFactory.getRelationType(outRow);
  types.bind(n, outRel);
  return outRel;
}

const Type *TypeInferrer::inferWhereRel(const WhereRel *n) {
  auto &types = ctx.getTypeContext();
  auto &typeFactory = types.getTypeFactory();

  // `where` keeps the input's schema, so it doesn't change the current row for
  // the next stage. Type the predicate against the input row.
  const Type *inputType =
      n->getInput() ? inferQuery(n->getInput()) : typeFactory.getErrorType();
  if (inputType->getKind() == TypeKind::Error) {
    types.bind(n, typeFactory.getErrorType());
    return typeFactory.getErrorType();
  }

  if (const Expr *predicate = n->getPredicate()) {
    const StructType *savedRow = currentRow;
    currentRow = rowOf(inputType);
    visit(predicate);
    currentRow = savedRow;

    const Type *predType = types.resolveType(types.typeOf(predicate));
    if (predType->getKind() != TypeKind::Bool &&
        predType->getKind() != TypeKind::Error) {
      ctx.error(predicate,
                llvm::formatv("`where` predicate must be `bool`, found `{0}`",
                              asString(predType->getKind()))
                    .str())
          .emit();
    }
  }

  // A filter doesn't change the schema — same relation type as the input.
  types.bind(n, inputType);
  return inputType;
}

const Type *TypeInferrer::inferDistinctRel(const DistinctRel *n) {
  auto &types = ctx.getTypeContext();
  auto &typeFactory = types.getTypeFactory();

  // `distinct` is transparent to scoping (like `where`): type the input in the
  // current scope so its columns flow through unchanged. Dedup keeps the
  // schema.
  const Type *inputType =
      n->getInput() ? inferQuery(n->getInput()) : typeFactory.getErrorType();
  if (inputType->getKind() == TypeKind::Error) {
    types.bind(n, typeFactory.getErrorType());
    return typeFactory.getErrorType();
  }

  types.bind(n, inputType);
  return inputType;
}

const Type *TypeInferrer::inferDropRel(const DropRel *n) {
  auto &types = ctx.getTypeContext();
  auto &typeFactory = types.getTypeFactory();

  const Type *inputType =
      n->getInput() ? inferQuery(n->getInput()) : typeFactory.getErrorType();
  if (inputType->getKind() == TypeKind::Error) {
    types.bind(n, typeFactory.getErrorType());
    return typeFactory.getErrorType();
  }
  const StructType *inputRow = rowOf(inputType);

  // Each dropped name must be a column of the input.
  for (const Ident *column : n->getColumns()) {
    if (inputRow->findField(column->getName()) == nullptr) {
      ctx.error(column, llvm::formatv("`drop` of unknown column `{0}`",
                                      util::toUtf8(column->getName()))
                            .str())
          .emit();
    }
  }

  // Output = input columns minus the dropped ones, order preserved.
  std::vector<StructField> outFields;
  for (const StructField &field : inputRow->getFields()) {
    bool dropped = false;
    for (const Ident *column : n->getColumns()) {
      if (column->getName() == field.name) {
        dropped = true;
        break;
      }
    }
    if (!dropped) {
      outFields.push_back(field);
    }
  }

  const StructType *outRow = typeFactory.getStructType(U"", outFields);
  const RelationType *outRel = typeFactory.getRelationType(outRow);
  types.bind(n, outRel);
  return outRel;
}

const Type *TypeInferrer::inferRenameRel(const RenameRel *n) {
  auto &types = ctx.getTypeContext();
  auto &typeFactory = types.getTypeFactory();

  const Type *inputType =
      n->getInput() ? inferQuery(n->getInput()) : typeFactory.getErrorType();
  if (inputType->getKind() == TypeKind::Error) {
    types.bind(n, typeFactory.getErrorType());
    return typeFactory.getErrorType();
  }
  const StructType *inputRow = rowOf(inputType);

  // Each renamed `from` must be a column of the input.
  for (const RenameItem *item : n->getItems()) {
    if (inputRow->findField(item->getFrom()->getName()) == nullptr) {
      ctx.error(item->getFrom(),
                llvm::formatv("`rename` of unknown column `{0}`",
                              util::toUtf8(item->getFrom()->getName()))
                    .str())
          .emit();
    }
  }

  // Output = input columns with renamed names substituted; positions preserved.
  std::vector<StructField> outFields;
  for (const StructField &field : inputRow->getFields()) {
    std::u32string_view name = field.name;
    for (const RenameItem *item : n->getItems()) {
      if (item->getFrom()->getName() == field.name) {
        name = item->getTo()->getName();
        break;
      }
    }
    outFields.push_back(StructField{name, field.type});
  }

  const StructType *outRow = typeFactory.getStructType(U"", outFields);
  const RelationType *outRel = typeFactory.getRelationType(outRow);
  types.bind(n, outRel);
  return outRel;
}

void TypeInferrer::visitFieldAccessExpr(const FieldAccessExpr *n) {
  auto &types = ctx.getTypeContext();
  auto &typeFactory = types.getTypeFactory();

  const Type *baseType = types.typeOf(n->getBase());
  if (baseType->getKind() == TypeKind::Error) {
    types.bind(n, typeFactory.getErrorType());
    return;
  }

  const auto *structType = StructType::cast(baseType);
  if (!structType) {
    ctx.error(n, llvm::formatv("type `{0}` has no fields",
                               asString(baseType->getKind()))
                     .str())
        .emit();
    types.bind(n, typeFactory.getErrorType());
    return;
  }

  const std::u32string_view field = n->getField()->getName();
  const Type *fieldType = structType->findField(field);
  if (!fieldType) {
    ctx.error(n, llvm::formatv("struct `{0}` has no field `{1}`",
                               util::toUtf8(structType->getName()),
                               util::toUtf8(field))
                     .str())
        .emit();
    types.bind(n, typeFactory.getErrorType());
    return;
  }

  types.bind(n, fieldType);
}

void TypeInferrer::visitStructExpr(const StructExpr *n) {
  auto &types = ctx.getTypeContext();
  auto &typeFactory = types.getTypeFactory();

  // The type name must resolve to a known struct.
  const std::u32string_view name = n->getName()->getName();
  const Type *type = ctx.getSymbolTable().lookupType(name);
  const auto *structType = type ? StructType::cast(type) : nullptr;
  if (!structType) {
    ctx.error(n,
              llvm::formatv("`{0}` is not a struct", util::toUtf8(name)).str())
        .emit();
    types.bind(n, typeFactory.getErrorType());
    return;
  }

  // Each initializer must name a declared field (no duplicates) and supply an
  // assignable value; every declared field must be initialized exactly once.
  llvm::SmallVector<std::u32string_view> seen;
  for (const StructFieldInit *field : n->getFields()) {
    const std::u32string_view fieldName = field->getName()->getName();
    const Type *fieldType = structType->findField(fieldName);
    if (!fieldType) {
      ctx.error(field,
                llvm::formatv("struct `{0}` has no field `{1}`",
                              util::toUtf8(name), util::toUtf8(fieldName))
                    .str())
          .emit();
      continue;
    }

    if (llvm::is_contained(seen, fieldName)) {
      ctx.error(field,
                llvm::formatv("field `{0}` is initialized more than once",
                              util::toUtf8(fieldName))
                    .str())
          .emit();
      continue;
    }

    seen.push_back(fieldName);

    if (!isAssignable(field->getValue(), fieldType)) {
      ctx.error(
             field->getValue(),
             llvm::formatv(
                 "value of type `{0}` is not assignable to field `{1}` of type "
                 "`{2}`",
                 asString(types.typeOf(field->getValue())->getKind()),
                 util::toUtf8(fieldName),
                 asString(types.resolveType(fieldType)->getKind()))
                 .str())
          .emit();
    }
  }

  for (const StructField &declared : structType->getFields()) {
    if (!llvm::is_contained(seen, declared.name)) {
      ctx.error(n,
                llvm::formatv("missing field `{0}` in `{1}` literal",
                              util::toUtf8(declared.name), util::toUtf8(name))
                    .str())
          .emit();
    }
  }

  types.bind(n, structType);
}

void TypeInferrer::visitReturnStmt(const ReturnStmt *returnStmt) {
  auto &types = ctx.getTypeContext();

  // Check only a value `return` inside a function with a declared return type.
  if (expectedReturn == nullptr || returnStmt->getExpr() == nullptr ||
      expectedReturn->getKind() == TypeKind::Error) {
    return;
  }

  // A poisoned return value already reported; don't cascade.
  const Expr *expr = returnStmt->getExpr();
  if (types.typeOf(expr)->getKind() == TypeKind::Error) {
    return;
  }

  if (!isAssignable(expr, expectedReturn)) {
    ctx.error(returnStmt,
              llvm::formatv("returning `{0}` from a function declared to "
                            "return `{1}`",
                            asString(types.typeOf(expr)->getKind()),
                            asString(expectedReturn->getKind()))
                  .str())
        .emit();
  }
}

const Type *
TypeInferrer::resolveTypeAnnotation(const TypeAnnotation *typeAnnotation,
                                    const HirNode *node) {
  if (!typeAnnotation) {
    return nullptr;
  }

  if (const auto *annotation = NamedTypeAnnotation::cast(typeAnnotation)) {
    return resolveNamedTypeAnnotation(annotation, node);
  }
  if (const auto *annotation = FuncTypeAnnotation::cast(typeAnnotation)) {
    return resolveFuncTypeAnnotation(annotation, node);
  }

  return ctx.getTypeContext().getTypeFactory().getErrorType();
}

const Type *
TypeInferrer::resolveFuncTypeAnnotation(const FuncTypeAnnotation *fnType,
                                        const HirNode *node) {
  auto &types = ctx.getTypeContext();

  std::vector<const Type *> params;
  params.reserve(fnType->getParams().size());
  for (const TypeAnnotation *param : fnType->getParams()) {
    params.push_back(resolveTypeAnnotation(param, node));
  }

  const Type *result = resolveTypeAnnotation(fnType->getResult(), node);
  return types.getTypeFactory().getFuncTy(params, result);
}

const Type *
TypeInferrer::resolveNamedTypeAnnotation(const NamedTypeAnnotation *annotation,
                                         const HirNode *node) {
  auto &types = ctx.getTypeContext();
  auto &symbolTable = ctx.getSymbolTable();

  const Ident *ident = annotation->getName();
  if (!ident) {
    ctx.error(node, "type is missing its name").emit();
    return types.getTypeFactory().getErrorType();
  }

  const std::u32string_view name = ident->getName();
  if (!annotation->getArgs().empty()) {
    ctx.error(node, llvm::formatv("parameterized types are not supported: "
                                  "`{0}` is not a generic type",
                                  util::toUtf8(name))
                        .str())
        .emit();
    return types.getTypeFactory().getErrorType();
  }

  if (const Type *type = symbolTable.lookupType(name)) {
    return type;
  }

  ctx.error(node, llvm::formatv("unknown type `{0}`", util::toUtf8(name)).str())
      .emit();

  return types.getTypeFactory().getErrorType();
}

} // namespace yuzu::hir