#include "../Hir/HirTestUtils.h"

#include "yuzu/Codegen/HirLocation.h"
#include "yuzu/Codegen/HirToMlir.h"
#include "yuzu/Codegen/Jit.h"

#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"

#include <gtest/gtest.h>

#include <string>

namespace {

using yuzu::codegen::lowerHirToMlir;
using yuzu::hir::test::HirFixture;

class HirToMlirTest : public HirFixture {
protected:
  std::string mlirString(std::u32string_view source) {
    const auto *hirRoot = lowerRoot(source);
    EXPECT_NE(hirRoot, nullptr);

    mlir::MLIRContext ctx;
    auto module = lowerHirToMlir(ctx, hirRoot);
    EXPECT_TRUE(module);
    EXPECT_TRUE(mlir::succeeded(mlir::verify(*module)));

    std::string out;
    llvm::raw_string_ostream os(out);
    module->print(os);
    return out;
  }
};

TEST_F(HirToMlirTest, EmptyRootReturnsZero) {
  const auto text = mlirString(U"");
  EXPECT_NE(text.find("func.func @yuzu_main() -> i64"), std::string::npos);
  EXPECT_NE(text.find("arith.constant 0 : i64"), std::string::npos);
  EXPECT_NE(text.find("return"), std::string::npos);
}

TEST_F(HirToMlirTest, LiteralReturnsConstant) {
  const auto text = mlirString(U"42");
  EXPECT_NE(text.find("arith.constant 42 : i64"), std::string::npos);
}

TEST_F(HirToMlirTest, BinaryAddLowersToAddi) {
  const auto text = mlirString(U"1 + 2");
  EXPECT_NE(text.find("arith.constant 1 : i64"), std::string::npos);
  EXPECT_NE(text.find("arith.constant 2 : i64"), std::string::npos);
  EXPECT_NE(text.find("arith.addi"), std::string::npos);
}

TEST_F(HirToMlirTest, BinaryDivLowersToDivSi) {
  const auto text = mlirString(U"8 / 2");
  EXPECT_NE(text.find("arith.divsi"), std::string::npos);
}

TEST_F(HirToMlirTest, MultipleStmtsKeepLast) {
  // Function returns the LAST expression's value — `2 + 3`.
  const auto text = mlirString(U"1\n2 + 3\n");
  EXPECT_NE(text.find("arith.addi"), std::string::npos);
}

class JitTest : public HirFixture {
protected:
  std::optional<int64_t> run(std::u32string_view source) {
    const auto *hirRoot = lowerRoot(source);
    EXPECT_NE(hirRoot, nullptr);

    mlir::MLIRContext ctx;
    auto module = yuzu::codegen::lowerHirToMlir(ctx, hirRoot);
    EXPECT_TRUE(module);
    return yuzu::codegen::jitExecute(ctx, *module);
  }
};

TEST_F(JitTest, OnePlusTwo) {
  EXPECT_EQ(run(U"1 + 2"), std::make_optional<int64_t>(3));
}

TEST_F(JitTest, NestedPrecedence) {
  // 1 + 2 * 3 = 7
  EXPECT_EQ(run(U"1 + 2 * 3"), std::make_optional<int64_t>(7));
}

TEST_F(JitTest, Subtraction) {
  EXPECT_EQ(run(U"10 - 7"), std::make_optional<int64_t>(3));
}

TEST_F(JitTest, Division) {
  EXPECT_EQ(run(U"20 / 4"), std::make_optional<int64_t>(5));
}

TEST_F(JitTest, ReturnsLastStmt) {
  EXPECT_EQ(run(U"1\n2 + 3\n"), std::make_optional<int64_t>(5));
}

TEST_F(JitTest, EmptyProgramReturnsZero) {
  EXPECT_EQ(run(U""), std::make_optional<int64_t>(0));
}

class HirLocationTest : public HirFixture {};

TEST_F(HirLocationTest, EncodeRoundTrip) {
  // OpaqueLoc<HirIdLocTag *>: encoding an arbitrary HirId and decoding
  // must yield the same value. Other Location kinds decode to nullopt.
  mlir::MLIRContext ctx;
  const auto id = yuzu::hir::HirId{7};
  const auto loc = yuzu::codegen::locFor(ctx, id);
  const auto recovered = yuzu::codegen::hirIdFromLoc(loc);
  ASSERT_TRUE(recovered.has_value());
  EXPECT_EQ(*recovered, id);

  EXPECT_FALSE(yuzu::codegen::hirIdFromLoc(mlir::UnknownLoc::get(&ctx))
                   .has_value());
}

TEST_F(HirLocationTest, LoweredOpsCarryHirIds) {
  // After lowering, each MLIR op for the `1 + 2` HIR should hold an
  // OpaqueLoc that decodes to the corresponding HIR node's id.
  const auto *hirRoot = lowerRoot(U"1 + 2");
  ASSERT_NE(hirRoot, nullptr);

  mlir::MLIRContext ctx;
  auto module = yuzu::codegen::lowerHirToMlir(ctx, hirRoot);
  ASSERT_TRUE(module);

  int decoded = 0;
  module->walk([&](mlir::Operation *op) {
    if (yuzu::codegen::hirIdFromLoc(op->getLoc())) {
      ++decoded;
    }
  });
  EXPECT_GT(decoded, 0);
}

} // namespace
