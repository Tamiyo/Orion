#ifndef YUZU_HIR_OP_OP_H
#define YUZU_HIR_OP_OP_H

#include "yuzu/Types/Type.h"

#include <llvm/ADT/ArrayRef.h>

#include <string_view>

namespace yuzu::hir {
class HirContext;
class Expr;

enum class OpKind {
  Builtin,
};

class [[nodiscard]] Op {
public:
  virtual ~Op() = default;

  [[nodiscard]] OpKind getKind() const { return kind; }

  /// Short human-readable name for the operator, used by printers and
  /// diagnostics ("Add", "Sub", ...). Subclasses override to expose
  /// their own `Name` constant.
  [[nodiscard]] virtual std::string_view getName() const = 0;

  /// Resolve the call's result type given its already-typed operands.
  /// Called during lowering so the constructed `CallExpr` carries the
  /// right type from the start. Subclasses implement per-operator rules
  /// (overload selection, coercion checks, error diagnosis).
  virtual const types::Type *resolve(llvm::ArrayRef<const Expr *> args,
                                     HirContext &ctx) const = 0;

protected:
  explicit Op(OpKind k) : kind(k) {}

private:
  OpKind kind;
};

} // namespace yuzu::hir

#endif // YUZU_HIR_OP_OP_H
