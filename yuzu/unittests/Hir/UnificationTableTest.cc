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
  EXPECT_TRUE(table.unify(hole, types.getInt32()));
  EXPECT_EQ(table.resolve(hole), types.getInt32());
}

TEST_F(UnificationTableTest, FillIsKindRestricted) {
  // An Int hole rejects a non-integer; a General hole accepts anything.
  EXPECT_FALSE(table.unify(table.makeTypeHole(InferKind::Int), types.getStr()));
  EXPECT_FALSE(table.unify(table.makeTypeHole(InferKind::Float), types.getInt32()));

  const auto *general = table.makeTypeHole(InferKind::General);
  EXPECT_TRUE(table.unify(general, types.getStr()));
  EXPECT_EQ(table.resolve(general), types.getStr());
}

TEST_F(UnificationTableTest, UnfilledNumericHolesDefault) {
  EXPECT_EQ(table.resolve(table.makeTypeHole(InferKind::Int)), types.getInt64());
  EXPECT_EQ(table.resolve(table.makeTypeHole(InferKind::Float)),
            types.getFloat64());
}

TEST_F(UnificationTableTest, UnfilledGeneralHoleIsError) {
  EXPECT_EQ(table.resolve(table.makeTypeHole(InferKind::General)),
            types.getError());
}

TEST_F(UnificationTableTest, ResolvePassesConcreteThrough) {
  EXPECT_EQ(table.resolve(types.getBool()), types.getBool());
}

TEST_F(UnificationTableTest, ConcreteVsConcrete) {
  EXPECT_TRUE(table.unify(types.getInt32(), types.getInt32()));
  EXPECT_FALSE(table.unify(types.getInt32(), types.getStr()));
}

//===----------------------------------------------------------------------===//
// Step B — hole-vs-hole linking (union-find).
//===----------------------------------------------------------------------===//

TEST_F(UnificationTableTest, LinkedHolesFillTogether) {
  // Two holes linked, then the group filled via one → both see it.
  const auto *a = table.makeTypeHole(InferKind::Int);
  const auto *b = table.makeTypeHole(InferKind::Int);
  EXPECT_TRUE(table.unify(a, b));
  EXPECT_TRUE(table.unify(b, types.getInt32()));
  EXPECT_EQ(table.resolve(a), types.getInt32());
  EXPECT_EQ(table.resolve(b), types.getInt32());
}

TEST_F(UnificationTableTest, LinkedHolesDefaultTogether) {
  // Linked but never filled → both default.
  const auto *a = table.makeTypeHole(InferKind::Int);
  const auto *b = table.makeTypeHole(InferKind::Int);
  EXPECT_TRUE(table.unify(a, b));
  EXPECT_EQ(table.resolve(a), types.getInt64());
  EXPECT_EQ(table.resolve(b), types.getInt64());
}

TEST_F(UnificationTableTest, GeneralHoleAdoptsNumericKind) {
  // `id(5)`: a General hole (T) unified with an Int hole (the literal).
  // The group is Int, so it defaults to int64 — not an unresolved error.
  const auto *t = table.makeTypeHole(InferKind::General);
  const auto *lit = table.makeTypeHole(InferKind::Int);
  EXPECT_TRUE(table.unify(t, lit));
  EXPECT_EQ(table.resolve(t), types.getInt64());
  EXPECT_EQ(table.resolve(lit), types.getInt64());
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
  EXPECT_TRUE(table.unify(a, types.getInt16()));
  EXPECT_EQ(table.resolve(c), types.getInt16());
}

} // namespace
