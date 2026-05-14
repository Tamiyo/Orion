#include "yuzu/Codegen/HirToMlir.h"

#include "yuzu/Codegen/HirLocation.h"
#include "yuzu/Util/ErrorHandling.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Value.h"

namespace yuzu::codegen {

namespace {

mlir::Value genExpr(mlir::OpBuilder &builder, const hir::Expr *expr) {
  auto &ctx = *builder.getContext();
  const auto loc = locFor(ctx, expr->getId());
  switch (expr->getKind()) {
  case hir::HirKind::LiteralExpr: {
    const auto *lit = hir::LiteralExpr::cast(expr);
    return builder.create<mlir::arith::ConstantIntOp>(loc, lit->getValue(),
                                                      builder.getI64Type());
  }
  case hir::HirKind::BinaryExpr: {
    const auto *bin = hir::BinaryExpr::cast(expr);
    mlir::Value lhs = genExpr(builder, bin->getLhs());
    mlir::Value rhs = genExpr(builder, bin->getRhs());
    switch (bin->getOp()) {
    case hir::BinOp::Add:
      return builder.create<mlir::arith::AddIOp>(loc, lhs, rhs);
    case hir::BinOp::Sub:
      return builder.create<mlir::arith::SubIOp>(loc, lhs, rhs);
    case hir::BinOp::Mul:
      return builder.create<mlir::arith::MulIOp>(loc, lhs, rhs);
    case hir::BinOp::Div:
      return builder.create<mlir::arith::DivSIOp>(loc, lhs, rhs);
    }
    util::yuzu_unreachable();
  }
  default:
    util::yuzu_unreachable();
  }
}

} // namespace

mlir::OwningOpRef<mlir::ModuleOp> lowerHirToMlir(mlir::MLIRContext &ctx,
                                                 const hir::Root *root) {
  ctx.loadDialect<mlir::arith::ArithDialect, mlir::func::FuncDialect>();

  mlir::OpBuilder builder(&ctx);
  const auto rootLoc = locFor(ctx, root->getId());

  auto module = mlir::ModuleOp::create(rootLoc);
  builder.setInsertionPointToStart(module.getBody());

  const auto i64Ty = builder.getI64Type();
  const auto fnTy =
      mlir::FunctionType::get(&ctx, /*inputs=*/{}, /*results=*/{i64Ty});
  auto fn = builder.create<mlir::func::FuncOp>(rootLoc, "yuzu_main", fnTy);

  auto *entry = fn.addEntryBlock();
  builder.setInsertionPointToStart(entry);

  // Each statement is currently an ExprStmt; the function's return value
  // is the last expression's value. Empty programs return 0.
  mlir::Value last;
  for (const hir::Stmt *stmt : root->getStmts()) {
    const auto *exprStmt = hir::ExprStmt::cast(stmt);
    last = genExpr(builder, exprStmt->getExpr());
  }
  if (!last) {
    last = builder.create<mlir::arith::ConstantIntOp>(rootLoc, 0, i64Ty);
  }
  builder.create<mlir::func::ReturnOp>(rootLoc, last);

  return module;
}

} // namespace yuzu::codegen
