#ifndef YUZU_OPS_BUILTINOP_H
#define YUZU_OPS_BUILTINOP_H

#include "yuzu/Util/ErrorHandling.h"

#include <string_view>

namespace yuzu {

/// The fixed set of builtin operators, shared by HIR and ANF. There is no
/// extension mechanism — every operator the language has is one of these. Each
/// is type-checked in HIR (`hir::resolve`) and constant-folded in ANF
/// (`anf::fold`); the lowerer carries the value across unchanged, so a
/// `CallExpr` in either layer stores a `BuiltinOp` directly.
enum class BuiltinOp {
  Add,
  Sub,
  Mul,
  Div,
  Pow,
  And,
  Or,
  In,
  NotIn,
  Eq,
  Neq,
  Lt,
  Lte,
  Gt,
  Gte,
  ShiftLeft,
  ShiftRight,
  UnaryPos,
  UnaryNeg,
  UnaryNot,
};

/// The operator's stable spelling, used by the HIR/ANF printers ("Add",
/// "UnaryNeg", ...). This is the operator's identity, not its source symbol.
[[nodiscard]] inline std::string_view name(BuiltinOp op) {
  switch (op) {
  case BuiltinOp::Add:
    return "Add";
  case BuiltinOp::Sub:
    return "Sub";
  case BuiltinOp::Mul:
    return "Mul";
  case BuiltinOp::Div:
    return "Div";
  case BuiltinOp::Pow:
    return "Pow";
  case BuiltinOp::And:
    return "And";
  case BuiltinOp::Or:
    return "Or";
  case BuiltinOp::In:
    return "In";
  case BuiltinOp::NotIn:
    return "NotIn";
  case BuiltinOp::Eq:
    return "Eq";
  case BuiltinOp::Neq:
    return "Neq";
  case BuiltinOp::Lt:
    return "Lt";
  case BuiltinOp::Lte:
    return "Lte";
  case BuiltinOp::Gt:
    return "Gt";
  case BuiltinOp::Gte:
    return "Gte";
  case BuiltinOp::ShiftLeft:
    return "ShiftLeft";
  case BuiltinOp::ShiftRight:
    return "ShiftRight";
  case BuiltinOp::UnaryPos:
    return "UnaryPos";
  case BuiltinOp::UnaryNeg:
    return "UnaryNeg";
  case BuiltinOp::UnaryNot:
    return "UnaryNot";
  }
  util::yuzu_unreachable("unknown BuiltinOp");
}

} // namespace yuzu

#endif // YUZU_OPS_BUILTINOP_H
