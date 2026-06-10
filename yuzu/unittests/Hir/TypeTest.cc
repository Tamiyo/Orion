#include "yuzu/Types/Type.h"

#include <gtest/gtest.h>

#include <cstdint>

namespace {

using namespace yuzu::types;

// `canRepresent` is overloaded on int64_t / double, so the test args are
// spelled `int64_t{...}` and double literals to bind the intended overload
// (a bare `int` would be ambiguous between the two).

TEST(TypeCanRepresentTest, SignedIntegerBounds) {
  EXPECT_TRUE(Int8Type().canRepresent(int64_t{127}));
  EXPECT_TRUE(Int8Type().canRepresent(int64_t{-128}));
  EXPECT_FALSE(Int8Type().canRepresent(int64_t{128}));
  EXPECT_FALSE(Int8Type().canRepresent(int64_t{-129}));

  EXPECT_TRUE(Int16Type().canRepresent(int64_t{32767}));
  EXPECT_TRUE(Int16Type().canRepresent(int64_t{-32768}));
  EXPECT_FALSE(Int16Type().canRepresent(int64_t{32768}));

  EXPECT_TRUE(Int32Type().canRepresent(int64_t{2147483647}));
  EXPECT_FALSE(Int32Type().canRepresent(int64_t{2147483648}));

  // int64 holds any int64_t.
  EXPECT_TRUE(Int64Type().canRepresent(INT64_MAX));
  EXPECT_TRUE(Int64Type().canRepresent(INT64_MIN));
}

TEST(TypeCanRepresentTest, UnsignedIntegerBounds) {
  EXPECT_TRUE(UInt8Type().canRepresent(int64_t{0}));
  EXPECT_TRUE(UInt8Type().canRepresent(int64_t{255}));
  EXPECT_FALSE(UInt8Type().canRepresent(int64_t{256}));
  EXPECT_FALSE(UInt8Type().canRepresent(int64_t{-1}));

  EXPECT_TRUE(UInt16Type().canRepresent(int64_t{65535}));
  EXPECT_FALSE(UInt16Type().canRepresent(int64_t{65536}));

  EXPECT_TRUE(UInt32Type().canRepresent(int64_t{4294967295}));
  EXPECT_FALSE(UInt32Type().canRepresent(int64_t{4294967296}));

  // uint64 holds any non-negative int64_t; negatives are rejected.
  EXPECT_TRUE(UInt64Type().canRepresent(INT64_MAX));
  EXPECT_FALSE(UInt64Type().canRepresent(int64_t{-1}));
}

TEST(TypeCanRepresentTest, FloatBounds) {
  EXPECT_TRUE(Float32Type().canRepresent(1.5));
  EXPECT_TRUE(Float32Type().canRepresent(-1.5));
  EXPECT_FALSE(Float32Type().canRepresent(1e40));
  EXPECT_FALSE(Float32Type().canRepresent(-1e40));

  // float64 holds any double.
  EXPECT_TRUE(Float64Type().canRepresent(1e308));
  EXPECT_TRUE(Float64Type().canRepresent(-1e308));
}

TEST(TypeCanRepresentTest, NonIntegerTypesRejectIntegers) {
  EXPECT_FALSE(Float32Type().canRepresent(int64_t{5}));
  EXPECT_FALSE(BoolType().canRepresent(int64_t{1}));
  EXPECT_FALSE(StrType().canRepresent(int64_t{0}));
}

TEST(TypeCanRepresentTest, NonFloatTypesRejectDoubles) {
  EXPECT_FALSE(Int32Type().canRepresent(1.5));
  EXPECT_FALSE(BoolType().canRepresent(1.0));
  EXPECT_FALSE(StrType().canRepresent(0.0));
}

} // namespace