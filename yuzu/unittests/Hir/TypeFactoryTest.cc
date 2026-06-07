#include "yuzu/Hir/Types/TypeFactory.h"

#include "yuzu/Hir/Types/Type.h"
#include "yuzu/Util/StringInterner.h"

#include <gtest/gtest.h>

namespace {

using namespace yuzu::hir;

// `TypeFactory` now depends on a `StringInterner` for name storage, so the
// fixture owns one and constructs the interner against it.
class TypeFactoryTest : public ::testing::Test {
protected:
  yuzu::util::StringInterner stringInterner;
  TypeFactory interner{stringInterner};
};

TEST_F(TypeFactoryTest, SameGetterReturnsStablePointer) {
  EXPECT_EQ(interner.getInt64Type(), interner.getInt64Type());
  EXPECT_EQ(interner.getFloat32Type(), interner.getFloat32Type());
  EXPECT_EQ(interner.getStrType(), interner.getStrType());
  EXPECT_EQ(interner.getErrorType(), interner.getErrorType());
}

TEST_F(TypeFactoryTest, EachGetterReturnsItsOwnKind) {
  EXPECT_EQ(interner.getInt8Type()->getKind(), TypeKind::Int8);
  EXPECT_EQ(interner.getInt16Type()->getKind(), TypeKind::Int16);
  EXPECT_EQ(interner.getInt32Type()->getKind(), TypeKind::Int32);
  EXPECT_EQ(interner.getInt64Type()->getKind(), TypeKind::Int64);
  EXPECT_EQ(interner.getUInt8Type()->getKind(), TypeKind::UInt8);
  EXPECT_EQ(interner.getUInt16Type()->getKind(), TypeKind::UInt16);
  EXPECT_EQ(interner.getUInt32Type()->getKind(), TypeKind::UInt32);
  EXPECT_EQ(interner.getUInt64Type()->getKind(), TypeKind::UInt64);
  EXPECT_EQ(interner.getFloat32Type()->getKind(), TypeKind::Float32);
  EXPECT_EQ(interner.getFloat64Type()->getKind(), TypeKind::Float64);
  EXPECT_EQ(interner.getBoolType()->getKind(), TypeKind::Bool);
  EXPECT_EQ(interner.getStrType()->getKind(), TypeKind::Str);
  EXPECT_EQ(interner.getUnitType()->getKind(), TypeKind::Unit);
  EXPECT_EQ(interner.getErrorType()->getKind(), TypeKind::Error);
}

TEST_F(TypeFactoryTest, RelationInternsByElement) {
  // Same element ⇒ same canonical `Relation` pointer; different elements
  // ⇒ different pointers. This is the interning contract extended to the
  // first compound type.
  const auto *intRel = interner.getRelationType(interner.getInt64Type());
  EXPECT_EQ(intRel, interner.getRelationType(interner.getInt64Type()));
  EXPECT_NE(intRel, interner.getRelationType(interner.getFloat32Type()));
}

TEST_F(TypeFactoryTest, RelationCarriesKindAndElement) {
  const auto *strRel = interner.getRelationType(interner.getStrType());
  EXPECT_EQ(strRel->getKind(), TypeKind::Relation);
  EXPECT_EQ(strRel->getElement(), interner.getStrType());

  // RTTI: a `Relation` casts to `RelationTy`; a primitive does not.
  EXPECT_NE(RelationType::cast(strRel), nullptr);
  EXPECT_EQ(RelationType::cast(interner.getStrType()), nullptr);
}

TEST_F(TypeFactoryTest, RelationOfRelationNestsAndInterns) {
  // `Relation[Relation[Int64]]` — the element of the outer relation is the
  // (interned) inner relation, and the whole thing still deduplicates.
  const auto *inner = interner.getRelationType(interner.getInt64Type());
  const auto *outer = interner.getRelationType(inner);
  EXPECT_EQ(outer->getElement(), inner);
  EXPECT_EQ(outer, interner.getRelationType(
                       interner.getRelationType(interner.getInt64Type())));
  EXPECT_NE(outer, inner);
}

TEST_F(TypeFactoryTest, GetScalarMapsKindToType) {
  // `getScalar` returns the interned scalar for each builtin `TypeKind`.
  // (Resolving a *name* to a type is the environment's job, not the factory's.)
  EXPECT_EQ(interner.getScalarTy(TypeKind::Int64), interner.getInt64Type());
  EXPECT_EQ(interner.getScalarTy(TypeKind::Bool), interner.getBoolType());
  EXPECT_EQ(interner.getScalarTy(TypeKind::Str), interner.getStrType());
  EXPECT_EQ(interner.getScalarTy(TypeKind::Float32), interner.getFloat32Type());
  EXPECT_EQ(interner.getScalarTy(TypeKind::Unit), interner.getUnitType());
}

TEST_F(TypeFactoryTest, GetStructExposesFields) {
  const StructField fields[] = {
      {U"id", interner.getStrType()},
      {U"tenure", interner.getInt64Type()},
  };
  const auto *employee = interner.getStructType(U"Employee", fields);

  // The struct carries its name and fields.
  EXPECT_EQ(employee->getKind(), TypeKind::Struct);
  EXPECT_EQ(employee->getName(), U"Employee");
  ASSERT_EQ(employee->getFields().size(), 2u);
  EXPECT_EQ(employee->findField(U"id"), interner.getStrType());
  EXPECT_EQ(employee->findField(U"tenure"), interner.getInt64Type());
  EXPECT_EQ(employee->findField(U"missing"), nullptr);

  // RTTI: a struct casts to `StructTy`, a primitive does not.
  EXPECT_NE(StructType::cast(employee), nullptr);
  EXPECT_EQ(StructType::cast(interner.getInt64Type()), nullptr);
}

TEST_F(TypeFactoryTest, GetStructCopiesNamesIntoArena) {
  // `getStruct` must copy the name and field names, so the type stays
  // valid after the caller's buffers are gone.
  const StructType *s = nullptr;
  {
    std::u32string structName = U"Temp";
    std::u32string fieldName = U"x";
    const StructField fields[] = {{fieldName, interner.getInt32Type()}};
    s = interner.getStructType(structName, fields);
    // Mutate the originals to prove the struct doesn't alias them.
    structName = U"clobbered";
    fieldName = U"clobbered";
  }
  EXPECT_EQ(s->getName(), U"Temp");
  ASSERT_EQ(s->getFields().size(), 1u);
  EXPECT_EQ(s->getFields()[0].name, U"x");
  EXPECT_EQ(s->findField(U"x"), interner.getInt32Type());
}

TEST_F(TypeFactoryTest, FuncCarriesParamsAndRet) {
  const Type *params[] = {interner.getInt32Type(), interner.getStrType()};
  const auto *fn = interner.getFuncTy(params, interner.getBoolType());

  EXPECT_EQ(fn->getKind(), TypeKind::Func);
  ASSERT_EQ(fn->getArgTypes().size(), 2u);
  EXPECT_EQ(fn->getArgTypes()[0], interner.getInt32Type());
  EXPECT_EQ(fn->getArgTypes()[1], interner.getStrType());
  EXPECT_EQ(fn->getReturnType(), interner.getBoolType());

  // RTTI: a func casts to `FuncTy`, a primitive does not.
  EXPECT_NE(FuncType::cast(fn), nullptr);
  EXPECT_EQ(FuncType::cast(interner.getInt32Type()), nullptr);
}

TEST_F(TypeFactoryTest, FuncWithNoParams) {
  const auto *fn = interner.getFuncTy({}, interner.getInt64Type());
  EXPECT_TRUE(fn->getArgTypes().empty());
  EXPECT_EQ(fn->getReturnType(), interner.getInt64Type());
}

TEST_F(TypeFactoryTest, FuncCopiesParamsIntoArena) {
  // `getFunc` copies the params array, so the type stays valid after the
  // caller's buffer is gone.
  const FuncType *fn = nullptr;
  {
    std::vector<const Type *> params = {interner.getInt8Type(),
                                        interner.getInt16Type()};
    fn = interner.getFuncTy(params, interner.getBoolType());
    params.clear();
    params.shrink_to_fit();
  }
  ASSERT_EQ(fn->getArgTypes().size(), 2u);
  EXPECT_EQ(fn->getArgTypes()[0], interner.getInt8Type());
  EXPECT_EQ(fn->getArgTypes()[1], interner.getInt16Type());
}

TEST_F(TypeFactoryTest, FuncIsNotInterned) {
  // Unlike `Relation`, function types are not deduplicated — each call
  // allocates a fresh `FuncTy` (so a generic signature's holes stay
  // distinct per instantiation). Structurally-identical calls differ by
  // pointer but agree on contents.
  const auto *a =
      interner.getFuncTy({interner.getInt32Type()}, interner.getBoolType());
  const auto *b =
      interner.getFuncTy({interner.getInt32Type()}, interner.getBoolType());
  EXPECT_NE(a, b);
  EXPECT_EQ(a->getReturnType(), b->getReturnType());
}

TEST_F(TypeFactoryTest, TypeParamCarriesIndexAndName) {
  const auto *t = interner.getTypeParamType(0, U"T");
  EXPECT_EQ(t->getKind(), TypeKind::TypeParam);
  EXPECT_EQ(t->getIndex(), 0u);
  EXPECT_EQ(t->getName(), U"T");

  // RTTI: a type param casts to `TypeParamTy`, a primitive does not.
  EXPECT_NE(TypeParamType::cast(t), nullptr);
  EXPECT_EQ(TypeParamType::cast(interner.getInt32Type()), nullptr);
}

TEST_F(TypeFactoryTest, TypeParamIsNotInterned) {
  // Each declared `[T]` is a distinct marker — identity, not name, matters.
  EXPECT_NE(interner.getTypeParamType(0, U"T"),
            interner.getTypeParamType(0, U"T"));
}

TEST_F(TypeFactoryTest, TypeParamCopiesNameIntoArena) {
  const TypeParamType *t = nullptr;
  {
    std::u32string name = U"Elem";
    t = interner.getTypeParamType(2, name);
    name = U"clobbered";
  }
  EXPECT_EQ(t->getName(), U"Elem");
  EXPECT_EQ(t->getIndex(), 2u);
}

TEST_F(TypeFactoryTest, DistinctPrimitivesHaveDistinctPointers) {
  // Pointer-equality is the interning contract, so every primitive must
  // be a distinct address from every other primitive.
  const Type *all[] = {
      interner.getInt8Type(),    interner.getInt16Type(),
      interner.getInt32Type(),   interner.getInt64Type(),
      interner.getUInt8Type(),   interner.getUInt16Type(),
      interner.getUInt32Type(),  interner.getUInt64Type(),
      interner.getFloat32Type(), interner.getFloat64Type(),
      interner.getBoolType(),    interner.getStrType(),
      interner.getUnitType(),    interner.getErrorType(),
  };
  for (std::size_t i = 0; i < std::size(all); ++i) {
    for (std::size_t j = i + 1; j < std::size(all); ++j) {
      EXPECT_NE(all[i], all[j])
          << "primitives at indices " << i << " and " << j
          << " share a pointer (kinds " << asString(all[i]->getKind()) << " / "
          << asString(all[j]->getKind()) << ")";
    }
  }
}

} // namespace
