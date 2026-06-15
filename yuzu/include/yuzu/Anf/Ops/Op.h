#ifndef YUZU_ANF_OPS_OP_H
#define YUZU_ANF_OPS_OP_H

#include <llvm/ADT/ArrayRef.h>

#include <string_view>

namespace yuzu::types {
class Type;
}

namespace yuzu::anf {

class Atom;
class Constant;
class AnfContext;

enum class OpKind {
  Builtin,
};

/// A resolved operator referenced by a `CallExpr`. ANF is already typed, so —
/// unlike `hir::Op` — an `Op` carries no `resolve`: it is identity plus a name
/// for printing and Substrait emission, and the constant-folding rule for its
/// own semantics.
class [[nodiscard]] Op {
public:
  virtual ~Op() = default;

  [[nodiscard]] OpKind getKind() const { return kind; }
  [[nodiscard]] virtual std::string_view getName() const = 0;

  /// Constant-fold this op applied to `args` that have already been resolved to
  /// their defining values. Returns the result constant, or null when it can't
  /// fold — a non-constant operand, an unsupported op, or a case that would
  /// trap at runtime (e.g. integer division by zero). `type` is the result
  /// type carried on the `CallExpr`.
  [[nodiscard]] virtual const Constant *
  fold(llvm::ArrayRef<const Atom *> /*args*/, AnfContext & /*ctx*/,
       const types::Type * /*type*/) const {
    return nullptr;
  }

protected:
  explicit Op(OpKind k) : kind(k) {}

private:
  OpKind kind;
};

} // namespace yuzu::anf

#endif // YUZU_ANF_OPS_OP_H
