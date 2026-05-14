#include "yuzu/Codegen/Jit.h"

#include "mlir/Conversion/ArithToLLVM/ArithToLLVM.h"
#include "mlir/Conversion/ControlFlowToLLVM/ControlFlowToLLVM.h"
#include "mlir/Conversion/FuncToLLVM/ConvertFuncToLLVMPass.h"
#include "mlir/Conversion/ReconcileUnrealizedCasts/ReconcileUnrealizedCasts.h"
#include "mlir/Conversion/SCFToControlFlow/SCFToControlFlow.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/ExecutionEngine/ExecutionEngine.h"
#include "mlir/ExecutionEngine/OptUtils.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Target/LLVMIR/Dialect/Builtin/BuiltinToLLVMIRTranslation.h"
#include "mlir/Target/LLVMIR/Dialect/LLVMIR/LLVMToLLVMIRTranslation.h"

#include "llvm/Support/TargetSelect.h"

namespace yuzu::codegen {

namespace {

bool lowerToLlvm(mlir::ModuleOp module) {
  mlir::PassManager pm(module.getContext());
  pm.addPass(mlir::createArithToLLVMConversionPass());
  pm.addPass(mlir::createConvertSCFToCFPass());
  pm.addPass(mlir::createConvertControlFlowToLLVMPass());
  pm.addPass(mlir::createConvertFuncToLLVMPass());
  pm.addPass(mlir::createReconcileUnrealizedCastsPass());
  return mlir::succeeded(pm.run(module));
}

void ensureNativeTargetInitialized() {
  static const bool done = [] {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    return true;
  }();
  (void)done;
}

} // namespace

std::optional<int64_t> jitExecute(mlir::MLIRContext &ctx,
                                  mlir::ModuleOp module) {
  if (!lowerToLlvm(module)) {
    return std::nullopt;
  }

  // Register dialect translations so the ExecutionEngine can produce
  // LLVM IR from the now-LLVM-dialect module.
  mlir::registerBuiltinDialectTranslation(ctx);
  mlir::registerLLVMDialectTranslation(ctx);

  ensureNativeTargetInitialized();

  // `options.transformer` is a non-owning `function_ref` — bind it to a
  // named local so its backing std::function lives across the
  // `ExecutionEngine::create` call.
  auto transformer = mlir::makeOptimizingTransformer(
      /*optLevel=*/0, /*sizeLevel=*/0, /*targetMachine=*/nullptr);
  mlir::ExecutionEngineOptions options;
  options.transformer = transformer;
  auto engineOrErr = mlir::ExecutionEngine::create(module, options);
  if (!engineOrErr) {
    llvm::consumeError(engineOrErr.takeError());
    return std::nullopt;
  }
  auto &engine = *engineOrErr;

  auto fnAddrOrErr = engine->lookup("yuzu_main");
  if (!fnAddrOrErr) {
    llvm::consumeError(fnAddrOrErr.takeError());
    return std::nullopt;
  }

  auto fn = reinterpret_cast<int64_t (*)()>(*fnAddrOrErr);
  return fn();
}

} // namespace yuzu::codegen
