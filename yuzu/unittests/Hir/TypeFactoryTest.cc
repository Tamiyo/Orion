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
  EXPECT_EQ(interner.getInt64(), interner.getInt64());
  EXPECT_EQ(interner.getFloat32(), interner.getFloat32());
  EXPECT_EQ(interner.getStr(), interner.getStr());
  EXPECT_EQ(interner.getError(), interner.getError());
}

TEST_F(TypeFactoryTest, EachGetterReturnsItsOwnKind) {
  EXPECT_EQ(interner.getInt8()->getKind(), TypeKind::Int8);
  EXPECT_EQ(interner.getInt16()->getKind(), TypeKind::Int16);
  EXPECT_EQ(interner.getInt32()->getKind(), TypeKind::Int32);
  EXPECT_EQ(interner.getInt64()->getKind(), TypeKind::Int64);
  EXPECT_EQ(interner.getUInt8()->getKind(), TypeKind::UInt8);
  EXPECT_EQ(interner.getUInt16()->getKind(), TypeKind::UInt16);
  EXPECT_EQ(interner.getUInt32()->getKind(), TypeKind::UInt32);
  EXPECT_EQ(interner.getUInt64()->getKind(), TypeKind::UInt64);
  EXPECT_EQ(interner.getFloat32()->getKind(), TypeKind::Float32);
  EXPECT_EQ(interner.getFloat64()->getKind(), TypeKind::Float64);
  EXPECT_EQ(interner.getBool()->getKind(), TypeKind::Bool);
  EXPECT_EQ(interner.getStr()->getKind(), TypeKind::Str);
  EXPECT_EQ(interner.getError()->getKind(), TypeKind::Error);
}

TEST_F(TypeFactoryTest, RelationInternsByElement) {
  // Same element ⇒ same canonical `Relation` pointer; different elements
  // ⇒ different pointers. This is the interning contract extended to the
  // first compound type.
  const auto *intRel = interner.getRelation(interner.getInt64());
  EXPECT_EQ(intRel, interner.getRelation(interner.getInt64()));
  EXPECT_NE(intRel, interner.getRelation(interner.getFloat32()));
}

TEST_F(TypeFactoryTest, RelationCarriesKindAndElement) {
  const auto *strRel = interner.getRelation(interner.getStr());
  EXPECT_EQ(strRel->getKind(), TypeKind::Relation);
  EXPECT_EQ(strRel->getElement(), interner.getStr());

  // RTTI: a `Relation` casts to `RelationTy`; a primitive does not.
  EXPECT_NE(RelationTy::cast(strRel), nullptr);
  EXPECT_EQ(RelationTy::cast(interner.getStr()), nullptr);
}

TEST_F(TypeFactoryTest, RelationOfRelationNestsAndInterns) {
  // `Relation[Relation[Int64]]` — the element of the outer relation is the
  // (interned) inner relation, and the whole thing still deduplicates.
  const auto *inner = interner.getRelation(interner.getInt64());
  const auto *outer = interner.getRelation(inner);
  EXPECT_EQ(outer->getElement(), inner);
  EXPECT_EQ(outer,
            interner.getRelation(interner.getRelation(interner.getInt64())));
  EXPECT_NE(outer, inner);
}

TEST_F(TypeFactoryTest, ResolvesBuiltinNames) {
  // The built-in spellings resolve to their primitive types; an unknown
  // name resolves to null (the lowerer turns that into a diagnostic).
  EXPECT_EQ(interner.resolveNamed(U"int64"), interner.getInt64());
  EXPECT_EQ(interner.resolveNamed(U"bool"), interner.getBool());
  EXPECT_EQ(interner.resolveNamed(U"str"), interner.getStr());
  EXPECT_EQ(interner.resolveNamed(U"float32"), interner.getFloat32());
  EXPECT_EQ(interner.resolveNamed(U"Employee"), nullptr);
}

TEST_F(TypeFactoryTest, GetStructRegistersAndExposesFields) {
  const Field fields[] = {
      {U"id", interner.getStr()},
      {U"tenure", interner.getInt64()},
  };
  const auto *employee = interner.getStruct(U"Employee", fields);

  // The struct resolves by name and carries its fields.
  EXPECT_EQ(interner.resolveNamed(U"Employee"), employee);
  EXPECT_EQ(employee->getKind(), TypeKind::Struct);
  EXPECT_EQ(employee->getName(), U"Employee");
  ASSERT_EQ(employee->getFields().size(), 2u);
  EXPECT_EQ(employee->findField(U"id"), interner.getStr());
  EXPECT_EQ(employee->findField(U"tenure"), interner.getInt64());
  EXPECT_EQ(employee->findField(U"missing"), nullptr);

  // RTTI: a struct casts to `StructTy`, a primitive does not.
  EXPECT_NE(StructTy::cast(employee), nullptr);
  EXPECT_EQ(StructTy::cast(interner.getInt64()), nullptr);
}

TEST_F(TypeFactoryTest, GetStructCopiesNamesIntoArena) {
  // `getStruct` must copy the name and field names, so the type stays
  // valid after the caller's buffers are gone.
  const StructTy *s = nullptr;
  {
    std::u32string structName = U"Temp";
    std::u32string fieldName = U"x";
    const Field fields[] = {{fieldName, interner.getInt32()}};
    s = interner.getStruct(structName, fields);
    // Mutate the originals to prove the struct doesn't alias them.
    structName = U"clobbered";
    fieldName = U"clobbered";
  }
  EXPECT_EQ(s->getName(), U"Temp");
  ASSERT_EQ(s->getFields().size(), 1u);
  EXPECT_EQ(s->getFields()[0].name, U"x");
  EXPECT_EQ(s->findField(U"x"), interner.getInt32());
}

TEST_F(TypeFactoryTest, DistinctPrimitivesHaveDistinctPointers) {
  // Pointer-equality is the interning contract, so every primitive must
  // be a distinct address from every other primitive.
  const Type *all[] = {
      interner.getInt8(),    interner.getInt16(),  interner.getInt32(),
      interner.getInt64(),   interner.getUInt8(),  interner.getUInt16(),
      interner.getUInt32(),  interner.getUInt64(), interner.getFloat32(),
      interner.getFloat64(), interner.getBool(),   interner.getStr(),
      interner.getError(),
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
