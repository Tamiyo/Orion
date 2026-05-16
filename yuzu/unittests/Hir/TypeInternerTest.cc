#include "yuzu/Hir/Types/TypeInterner.h"

#include "yuzu/Hir/Types/Type.h"

#include <gtest/gtest.h>

namespace {

using namespace yuzu::hir;

TEST(TypeInternerTest, SameGetterReturnsStablePointer) {
  TypeInterner interner;
  EXPECT_EQ(interner.getInt64(), interner.getInt64());
  EXPECT_EQ(interner.getFloat32(), interner.getFloat32());
  EXPECT_EQ(interner.getStr(), interner.getStr());
  EXPECT_EQ(interner.getError(), interner.getError());
}

TEST(TypeInternerTest, EachGetterReturnsItsOwnKind) {
  TypeInterner interner;
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

TEST(TypeInternerTest, DistinctPrimitivesHaveDistinctPointers) {
  // Pointer-equality is the interning contract, so every primitive must
  // be a distinct address from every other primitive.
  TypeInterner interner;
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
