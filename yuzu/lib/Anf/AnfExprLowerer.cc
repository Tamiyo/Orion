#include "yuzu/Anf/AnfLowerer.h"

#include "yuzu/Anf/Anf.h"
#include "yuzu/Anf/Ops/BuiltinOps.h"
#include "yuzu/Anf/Ops/Op.h"
#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/Ops/BuiltinOps.h"
#include "yuzu/Hir/Ops/Op.h"

namespace yuzu::anf {
const Expr *AnfLowerer::lowerExpr(const hir::Expr *expr) {
  switch (expr->getExprKind()) {
  case hir::ExprKind::IdentExpr:
    return lowerIdentExpr(hir::IdentExpr::cast(expr));
  case hir::ExprKind::CallExpr:
    return lowerCallExpr(hir::CallExpr::cast(expr));
  case hir::ExprKind::FuncCallExpr:
    return lowerFuncCallExpr(hir::FuncCallExpr::cast(expr));
  case hir::ExprKind::Rel:
    return lowerRel(hir::Rel::cast(expr));
  case hir::ExprKind::FieldAccessExpr:
    return lowerFieldAccessExpr(hir::FieldAccessExpr::cast(expr));
  case hir::ExprKind::StructExpr:
    return lowerStructExpr(hir::StructExpr::cast(expr));
  case hir::ExprKind::Literal:
    return lowerLiteral(hir::Literal::cast(expr));
  }
}

const Expr *AnfLowerer::lowerIdentExpr(const hir::IdentExpr *identExpr) {
  // Resolve an hir::Ident to the binding it corresponds to.
  //
  // For example:
  // ```
  // let x = 5
  // print(x)
  //       ^
  // ```
  //
  // The variable `x` in `print(x)` resolves to `let x = 5`.
  const auto *resolved = hirCtx.resolveIdent(identExpr->getName());

  // Map that HIR declaration to the ANF Binding we built for it.
  //
  // For example:
  // ```
  // let x = 5
  // print(x)
  //       ^
  // ```
  // The HIR decl `let x = 5` maps to the ANF binding `x`, which this
  // VarAtom points at.
  const auto *binding = hirToAnfBindingMap.lookup(resolved);

  // Resolve the type of the ident.
  const auto *type = hirCtx.getTypeContext().typeOf(identExpr);
  return ctx.build(identExpr, &AnfBuilder::makeVarAtom, binding, type);
}

const Expr *AnfLowerer::lowerCallExpr(const hir::CallExpr *callExpr) {
  std::vector<const Atom *> loweredArgs;
  loweredArgs.reserve(callExpr->getArgs().size());

  for (const auto *arg : callExpr->getArgs()) {
    const auto *atom = forceAtom(arg);
    loweredArgs.push_back(atom);
  }

  const auto *op = lowerOp(callExpr->getOp());
  const auto *type = hirCtx.getTypeContext().typeOf(callExpr);
  return ctx.build(callExpr, &AnfBuilder::makeCallExpr, op, loweredArgs, type);
}

const Expr *
AnfLowerer::lowerFuncCallExpr(const hir::FuncCallExpr *funcCallExpr) {
  const auto *callee = resolveCallee(funcCallExpr->getCallee());

  std::vector<const Atom *> loweredArgs;
  loweredArgs.reserve(funcCallExpr->getArgs().size());
  for (const auto *arg : funcCallExpr->getArgs())
    loweredArgs.push_back(forceAtom(arg));

  const auto *type = hirCtx.getTypeContext().typeOf(funcCallExpr);
  return ctx.build(funcCallExpr, &AnfBuilder::makeFuncCallExpr, callee,
                   loweredArgs, type);
}

// A direct call `f(...)` (callee is an identifier resolving to a function we've
// already lowered) becomes a `FuncRef` so inlining can chase the target. Any
// other callee — an indirect call through a value, or a not-yet-lowered forward
// reference — keeps an atom callee via `forceAtom`.
const Atom *AnfLowerer::resolveCallee(const hir::Expr *callee) {
  if (const auto *ident = hir::IdentExpr::cast(callee)) {
    const auto *decl = hirCtx.resolveIdent(ident->getName());
    if (const auto *func = hirToAnfFuncMap.lookup(decl)) {
      const auto *type = hirCtx.getTypeContext().typeOf(callee);
      return ctx.build(callee, &AnfBuilder::makeFuncRef, func, type);
    }
  }
  return forceAtom(callee);
}

const Expr *
AnfLowerer::lowerFieldAccessExpr(const hir::FieldAccessExpr *fieldAccessExpr) {
  const auto *base = forceAtom(fieldAccessExpr->getBase());
  const auto *field = lowerIdent(fieldAccessExpr->getField());
  const auto *type = hirCtx.getTypeContext().typeOf(fieldAccessExpr);
  return ctx.build(fieldAccessExpr, &AnfBuilder::makeFieldAtom, base, field,
                   type);
}

const Expr *AnfLowerer::lowerStructExpr(const hir::StructExpr *structExpr) {
  std::vector<const StructFieldInit *> loweredFields;
  loweredFields.reserve(structExpr->getFields().size());
  for (const auto *field : structExpr->getFields()) {
    const auto *fieldName = lowerIdent(field->getName());
    const auto *fieldValue = forceAtom(field->getValue());
    loweredFields.push_back(ctx.build(field, &AnfBuilder::makeStructFieldInit,
                                      fieldName, fieldValue));
  }

  const auto *name = lowerIdent(structExpr->getName());
  const auto *type = hirCtx.getTypeContext().typeOf(structExpr);
  return ctx.build(structExpr, &AnfBuilder::makeStructExpr, name, loweredFields,
                   type);
}

const Constant *AnfLowerer::lowerLiteral(const hir::Literal *literal) {
  switch (literal->getLiteralKind()) {
  case hir::LiteralKind::BoolLit:
    return lowerBoolLit(hir::BoolLit::cast(literal));
  case hir::LiteralKind::IntLit:
    return lowerIntLit(hir::IntLit::cast(literal));
  case hir::LiteralKind::FloatLit:
    return lowerFloatLit(hir::FloatLit::cast(literal));
  case hir::LiteralKind::StringLit:
    return lowerStringLit(hir::StringLit::cast(literal));
  }
}

const BoolConst *AnfLowerer::lowerBoolLit(const hir::BoolLit *boolLit) {
  const auto value = boolLit->getValue();
  const auto *type = hirCtx.getTypeContext().typeOf(boolLit);
  return ctx.build(boolLit, &AnfBuilder::makeBoolConst, value, type);
}

const IntConst *AnfLowerer::lowerIntLit(const hir::IntLit *intLit) {
  const auto value = intLit->getValue();
  const auto *type = hirCtx.getTypeContext().typeOf(intLit);
  return ctx.build(intLit, &AnfBuilder::makeIntConst, value, type);
}

const FloatConst *AnfLowerer::lowerFloatLit(const hir::FloatLit *floatLit) {
  const auto value = floatLit->getValue();
  const auto *type = hirCtx.getTypeContext().typeOf(floatLit);
  return ctx.build(floatLit, &AnfBuilder::makeFloatConst, value, type);
}

const StringConst *AnfLowerer::lowerStringLit(const hir::StringLit *stringLit) {
  const auto value = stringLit->getValue();
  const auto *type = hirCtx.getTypeContext().typeOf(stringLit);
  return ctx.build(stringLit, &AnfBuilder::makeStringConst, value, type);
}

const Op *AnfLowerer::lowerOp(const hir::Op *op) {
  static const std::unordered_map<std::string_view, const Op *> table = {
      {hir::AddOp::Name, AddOp::get()},
      {hir::AndOp::Name, AndOp::get()},
      {hir::SubOp::Name, SubOp::get()},
      {hir::MulOp::Name, MulOp::get()},
      {hir::DivOp::Name, DivOp::get()},
      {hir::OrOp::Name, OrOp::get()},
      {hir::InOp::Name, InOp::get()},
      {hir::NotInOp::Name, NotInOp::get()},
      {hir::PowOp::Name, PowOp::get()},
      {hir::EqOp::Name, EqOp::get()},
      {hir::NeqOp::Name, NeqOp::get()},
      {hir::LtOp::Name, LtOp::get()},
      {hir::LteOp::Name, LteOp::get()},
      {hir::GtOp::Name, GtOp::get()},
      {hir::GteOp::Name, GteOp::get()},
      {hir::ShiftLeftOp::Name, ShiftLeftOp::get()},
      {hir::ShiftRightOp::Name, ShiftRightOp::get()},
      {hir::UnaryPosOp::Name, UnaryPosOp::get()},
      {hir::UnaryNegOp::Name, UnaryNegOp::get()},
      {hir::UnaryNotOp::Name, UnaryNotOp::get()},
  };
  const auto it = table.find(op->getName());
  return it != table.end() ? it->second : nullptr;
}

} // namespace yuzu::anf