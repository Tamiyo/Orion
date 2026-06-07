#include "yuzu/Hir/Types/TypeUnifier.h"

#include "yuzu/Hir/Types/Type.h"
#include "yuzu/Hir/Types/TypeFactory.h"
#include "yuzu/Util/StringInterner.h"

#include <gtest/gtest.h>

namespace {

using namespace yuzu::hir;

class UnificationTableTest : public ::testing::Test {
protected:
  yuzu::util::StringInterner strings;
  TypeFactory types{strings};
  TypeUnifier table{types};
};

TEST_F(UnificationTableTest, MakeHoleCarriesItsKind) {
  const auto *hole = table.makeTypeHole(InferKind::Int);
  EXPECT_EQ(hole->getKind(), TypeKind::Infer);
  EXPECT_EQ(hole->getInferKind(), InferKind::Int);
}

TEST_F(UnificationTableTest, FillsHoleFromConcrete) {
  const auto *hole = table.makeTypeHole(InferKind::Int);
  EXPECT_TRUE(table.unify(hole, types.getInt32Type()));
  EXPECT_EQ(table.resolve(hole), types.getInt32Type());
}

TEST_F(UnificationTableTest, FillIsKindRestricted) {
  // An Int hole rejects a non-integer; a General hole accepts anything.
  EXPECT_FALSE(table.unify(table.makeTypeHole(InferKind::Int), types.getStrType()));
  EXPECT_FALSE(table.unify(table.makeTypeHole(InferKind::Float), types.getInt32Type()));

  const auto *general = table.makeTypeHole(InferKind::General);
  EXPECT_TRUE(table.unify(general, types.getStrType()));
  EXPECT_EQ(table.resolve(general), types.getStrType());
}

TEST_F(UnificationTableTest, UnfilledNumericHolesDefault) {
  EXPECT_EQ(table.resolve(table.makeTypeHole(InferKind::Int)), types.getInt64Type());
  EXPECT_EQ(table.resolve(table.makeTypeHole(InferKind::Float)),
            types.getFloat64Type());
}

TEST_F(UnificationTableTest, UnfilledGeneralHoleIsError) {
  EXPECT_EQ(table.resolve(table.makeTypeHole(InferKind::General)),
            types.getErrorType());
}

TEST_F(UnificationTableTest, ResolvePassesConcreteThrough) {
  EXPECT_EQ(table.resolve(types.getBoolType()), types.getBoolType());
}

TEST_F(UnificationTableTest, ConcreteVsConcrete) {
  EXPECT_TRUE(table.unify(types.getInt32Type(), types.getInt32Type()));
  EXPECT_FALSE(table.unify(types.getInt32Type(), types.getStrType()));
}

//===----------------------------------------------------------------------===//
// Function types — not interned, so structurally-equal signatures are
// distinct pointers and must unify component-wise.
//===----------------------------------------------------------------------===//

TEST_F(UnificationTableTest, StructurallyEqualFuncTypesUnify) {
  const auto *a = types.getFuncTy({types.getInt32Type()}, types.getBoolType());
  const auto *b = types.getFuncTy({types.getInt32Type()}, types.getBoolType());
  EXPECT_NE(a, b); // distinct pointers — confirms they aren't interned
  EXPECT_TRUE(table.unify(a, b));
}

TEST_F(UnificationTableTest, FuncTypesDifferingInParamDoNotUnify) {
  const auto *a = types.getFuncTy({types.getInt32Type()}, types.getBoolType());
  const auto *b = types.getFuncTy({types.getStrType()}, types.getBoolType());
  EXPECT_FALSE(table.unify(a, b));
}

TEST_F(UnificationTableTest, FuncTypesDifferingInResultDoNotUnify) {
  const auto *a = types.getFuncTy({types.getInt32Type()}, types.getBoolType());
  const auto *b = types.getFuncTy({types.getInt32Type()}, types.getInt32Type());
  EXPECT_FALSE(table.unify(a, b));
}

TEST_F(UnificationTableTest, FuncTypesDifferingInArityDoNotUnify) {
  const auto *a = types.getFuncTy({types.getInt32Type()}, types.getBoolType());
  const auto *b = types.getFuncTy(
      {types.getInt32Type(), types.getInt32Type()}, types.getBoolType());
  EXPECT_FALSE(table.unify(a, b));
}

// A hole inside a function type is filled by unifying with a concrete
// signature — the structural walk reaches nested holes.
TEST_F(UnificationTableTest, FuncTypeUnificationFillsNestedHole) {
  const auto *hole = table.makeTypeHole(InferKind::General);
  const auto *withHole = types.getFuncTy({hole}, types.getBoolType());
  const auto *concrete =
      types.getFuncTy({types.getInt32Type()}, types.getBoolType());
  EXPECT_TRUE(table.unify(withHole, concrete));
  EXPECT_EQ(table.resolve(hole), types.getInt32Type());
}

//===----------------------------------------------------------------------===//
// Step B — hole-vs-hole linking (union-find).
//===----------------------------------------------------------------------===//

TEST_F(UnificationTableTest, LinkedHolesFillTogether) {
  // Two holes linked, then the group filled via one → both see it.
  const auto *a = table.makeTypeHole(InferKind::Int);
  const auto *b = table.makeTypeHole(InferKind::Int);
  EXPECT_TRUE(table.unify(a, b));
  EXPECT_TRUE(table.unify(b, types.getInt32Type()));
  EXPECT_EQ(table.resolve(a), types.getInt32Type());
  EXPECT_EQ(table.resolve(b), types.getInt32Type());
}

TEST_F(UnificationTableTest, LinkedHolesDefaultTogether) {
  // Linked but never filled → both default.
  const auto *a = table.makeTypeHole(InferKind::Int);
  const auto *b = table.makeTypeHole(InferKind::Int);
  EXPECT_TRUE(table.unify(a, b));
  EXPECT_EQ(table.resolve(a), types.getInt64Type());
  EXPECT_EQ(table.resolve(b), types.getInt64Type());
}

TEST_F(UnificationTableTest, GeneralHoleAdoptsNumericKind) {
  // `id(5)`: a General hole (T) unified with an Int hole (the literal).
  // The group is Int, so it defaults to int64 — not an unresolved error.
  const auto *t = table.makeTypeHole(InferKind::General);
  const auto *lit = table.makeTypeHole(InferKind::Int);
  EXPECT_TRUE(table.unify(t, lit));
  EXPECT_EQ(table.resolve(t), types.getInt64Type());
  EXPECT_EQ(table.resolve(lit), types.getInt64Type());
}

TEST_F(UnificationTableTest, IntAndFloatHolesClash) {
  EXPECT_FALSE(table.unify(table.makeTypeHole(InferKind::Int),
                                table.makeTypeHole(InferKind::Float)));
}

TEST_F(UnificationTableTest, LinkChainPropagates) {
  // a—b—c linked, fill via a, c still sees it (chain + path compression).
  const auto *a = table.makeTypeHole(InferKind::Int);
  const auto *b = table.makeTypeHole(InferKind::Int);
  const auto *c = table.makeTypeHole(InferKind::Int);
  EXPECT_TRUE(table.unify(a, b));
  EXPECT_TRUE(table.unify(b, c));
  EXPECT_TRUE(table.unify(a, types.getInt16Type()));
  EXPECT_EQ(table.resolve(c), types.getInt16Type());
}

} // namespace
