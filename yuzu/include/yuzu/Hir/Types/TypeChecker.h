#ifndef YUZU_HIR_TYPES_TYPECHECKER_H
#define YUZU_HIR_TYPES_TYPECHECKER_H

#include "yuzu/Diagnostics/DiagnosticsEngine.h"
#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/HirContext.h"
#include "yuzu/Hir/Types/Type.h"

namespace yuzu::hir {
class TypeChecker final {
public:
  explicit TypeChecker(HirContext &hirContext,
                       diagnostics::DiagnosticsEngine &diagnostics,
                       diagnostics::SourceId sourceId)
      : ctx(hirContext), diagnostics(diagnostics), sourceId(sourceId) {}

  TypeChecker() = delete;

  void check(const Root *root);

private:
  void checkStmt(const Stmt *s);
  void checkExprStmt(const ExprStmt *s);
  void checkLetStmt(const LetStmt *s);

  const Type *checkExpr(const Expr *e);
  const Type *checkCallExpr(const CallExpr *e);

  const Type *checkLiteral(const Literal *l);

  [[maybe_unused]] HirContext &ctx;
  [[maybe_unused]] diagnostics::DiagnosticsEngine &diagnostics;
  [[maybe_unused]] diagnostics::SourceId sourceId;
};

} // namespace yuzu::hir

#endif // YUZU_HIR_TYPES_TYPE_CHECKER_H
