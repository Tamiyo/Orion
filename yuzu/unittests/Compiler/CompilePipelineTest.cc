#include "yuzu/Compiler/CompilePipeline.h"

#include <llvm/Support/raw_ostream.h>

#include <gtest/gtest.h>

#include <string>

namespace {

yuzu::CompileOptions opts(llvm::raw_ostream &os) {
  return yuzu::CompileOptions{
      .out = os,
      .execute = false,
      .debugLexer = false,
      .debugAst = false,
      .debugHir = true,
      .debugMlir = false,
  };
}

TEST(CompilePipelineTest, OnePlusTwoLowersToTypedCall) {
  // `1 + 2`: both operands are int64; the lowerer resolves the call
  // through `AddOp::resolve`, so the resulting CallExpr is typed as
  // int64 and the operands appear as int64 literals.
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"1 + 2", opts(os));

  EXPECT_NE(out.find("=== hir ==="), std::string::npos) << out;
  EXPECT_NE(out.find("CallExpr : int64"), std::string::npos) << out;
  EXPECT_NE(out.find("value=1"), std::string::npos) << out;
  EXPECT_NE(out.find("value=2"), std::string::npos) << out;
}

TEST(CompilePipelineTest, NestedPrecedenceLowersToNestedCall) {
  // `1 + 2 * 3` parses as `1 + (2 * 3)`. The HIR dump should show two
  // nested `CallExpr` nodes — the outer add and the inner multiply.
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"1 + 2 * 3", opts(os));

  const auto outer = out.find("CallExpr : int64");
  ASSERT_NE(outer, std::string::npos) << out;
  const auto inner = out.find("CallExpr : int64", outer + 1);
  EXPECT_NE(inner, std::string::npos)
      << "expected two CallExpr nodes for the nested precedence:\n"
      << out;
  EXPECT_NE(out.find("value=3"), std::string::npos) << out;
}

TEST(CompilePipelineTest, MixedNumericCoercesToFloat64) {
  // `1 + 2.0` mixes int64 and float64. `coerceTypes` picks float64
  // as the common type via the numeric promotion rank, so the
  // CallExpr is typed as float64 even though one operand is an
  // IntLit.
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"1 + 2.0", opts(os));

  EXPECT_NE(out.find("CallExpr : float64"), std::string::npos) << out;
  EXPECT_NE(out.find("IntLit : int64"), std::string::npos) << out;
  EXPECT_NE(out.find("FloatLit : float64"), std::string::npos) << out;
}

TEST(CompilePipelineTest, ParenGroupingFlipsAssociativity) {
  // `(1 + 2) * 3` forces the add to evaluate first — the inverse of
  // `1 + 2 * 3`. The HIR should show Mul as the outer call with Add
  // nested inside. This also locks in the lowering of `ParenExpr`
  // (regression: without the wrapping node, paren tokens leaked into
  // the outer `BinaryExpr` and lowering reported "binary expression is
  // missing its operator").
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"(1 + 2) * 3", opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos)
      << "paren grouping must not emit a diagnostic:\n"
      << out;
  const auto mul = out.find("op=Mul");
  const auto add = out.find("op=Add");
  ASSERT_NE(mul, std::string::npos) << out;
  ASSERT_NE(add, std::string::npos) << out;
  EXPECT_LT(mul, add) << "expected Mul (outer) before Add (inner):\n" << out;
}

TEST(CompilePipelineTest, IncompleteBinaryEmitsErrorAndDropsCall) {
  // A binary expression missing its rhs is rejected during lowering.
  // The pipeline emits an error diagnostic and the resulting HIR has
  // no `CallExpr` — only an empty `Root` since the broken statement is
  // dropped.
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"1 +", opts(os));

  EXPECT_NE(out.find("error:"), std::string::npos)
      << "expected an error diagnostic for the incomplete binary:\n"
      << out;
  EXPECT_EQ(out.find("CallExpr"), std::string::npos)
      << "incomplete binary should not produce a CallExpr in HIR:\n"
      << out;
}

//===----------------------------------------------------------------------===//
// `let` binding types — annotation lowered/checked via `lowerType` +
// `coercesTo`, with the resolved type recorded on the `LetStmt`.
//===----------------------------------------------------------------------===//

// With no annotation the binding type is inferred from the initializer.
TEST(CompilePipelineTest, InfersBindingTypeFromInitializer) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"let x = 5", opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos) << out;
  EXPECT_NE(out.find("LetStmt : int64"), std::string::npos) << out;
}

// A matching annotation is recorded as the binding type.
TEST(CompilePipelineTest, MatchingAnnotationIsBindingType) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"let x: int64 = 5", opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos) << out;
  EXPECT_NE(out.find("LetStmt : int64"), std::string::npos) << out;
}

// A wider annotation is accepted; the initializer gets a cast adjustment
// and the binding takes the annotated type.
TEST(CompilePipelineTest, WideningAnnotationCoercesInitializer) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"let x: float64 = 5", opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos) << out;
  EXPECT_NE(out.find("LetStmt : float64"), std::string::npos) << out;
  EXPECT_NE(out.find("cast float64"), std::string::npos) << out;
}

// A non-coercible annotation is a type error.
TEST(CompilePipelineTest, MismatchedAnnotationEmitsError) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"let x: str = 5", opts(os));

  EXPECT_NE(out.find("is not assignable to `str`"), std::string::npos) << out;
}

// An untyped integer literal adapts to a narrower annotation — `5` is an
// inference hole, so the annotation pins it to int32 directly (no cast).
TEST(CompilePipelineTest, LiteralAdaptsToNarrowerAnnotation) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"let x: int32 = 5", opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos) << out;
  EXPECT_NE(out.find("LetStmt : int32"), std::string::npos) << out;
  EXPECT_NE(out.find("IntLit : int32"), std::string::npos) << out;
}

// The whole expression adapts: both literals in `5 + 5` are holes the
// annotation pins to int32, so the add resolves to int32, not the int64
// default. This is the case literal-defaulting alone couldn't handle.
TEST(CompilePipelineTest, ExpressionAdaptsToAnnotation) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"let x: int32 = 5 + 5", opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos) << out;
  EXPECT_NE(out.find("LetStmt : int32"), std::string::npos) << out;
  EXPECT_NE(out.find("CallExpr : int32"), std::string::npos) << out;
}

// Narrowing a concrete *value* (not a literal) stays an error — `x` is a
// bound int64, which doesn't implicitly narrow to int32.
TEST(CompilePipelineTest, NarrowingConcreteValueEmitsError) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"let x: int64 = 5\nlet y: int32 = x", opts(os));

  EXPECT_NE(out.find("is not assignable to `int32`"), std::string::npos) << out;
}

// A use of a binding resolves to its *binding* type, not the
// initializer's: `x` is `float64` (the annotation), not `int64` (the
// literal), so `x + x` is `float64`.
TEST(CompilePipelineTest, IdentResolvesToBindingType) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"let x: float64 = 5\nlet y = x + x", opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos) << out;
  EXPECT_NE(out.find("IdentExpr : float64"), std::string::npos) << out;
  EXPECT_NE(out.find("LetStmt : float64\n    Ident\n      name=y"),
            std::string::npos)
      << out;
}

// An unknown type name is reported.
TEST(CompilePipelineTest, UnknownAnnotationTypeEmitsError) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"let x: bogus = 5", opts(os));

  EXPECT_NE(out.find("unknown type `bogus`"), std::string::npos) << out;
}

// A function-type annotation resolves to a `func` type; assigning a
// non-function to it is a normal type mismatch.
TEST(CompilePipelineTest, FunctionTypeAnnotationRejectsNonFunction) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"let x: (int64) -> bool = 5", opts(os));

  EXPECT_NE(out.find("value of type `int64` is not assignable to `func`"),
            std::string::npos)
      << out;
}

// A function whose signature matches the annotation is assignable to it —
// structurally-equal `func` types unify even though they aren't interned.
TEST(CompilePipelineTest, FunctionTypeAnnotationAcceptsMatchingFunction) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn add_one(y: int32) -> int32 { return y + 1 }\n"
                U"let f: (int32) -> int32 = add_one",
                opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos) << out;
}

// A function returning a function: the return annotation is itself a
// function type, and the returned function is checked against it.
TEST(CompilePipelineTest, FunctionReturningFunctionTypechecks) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn add_one(y: int32) -> int32 { return y + 1 }\n"
                U"fn get() -> (int32) -> int32 { return add_one }\n"
                U"let r = get()(7)",
                opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos) << out;
  EXPECT_NE(out.find("FuncCallExpr : int32"), std::string::npos) << out;
}

//===----------------------------------------------------------------------===//
// Literal range checks — a literal must fit the type it resolved to.
//===----------------------------------------------------------------------===//

// A signed integer literal beyond its annotated type's range is rejected.
TEST(CompilePipelineTest, IntLiteralOutOfRangeEmitsError) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"let x: int8 = 500", opts(os));

  EXPECT_NE(out.find("integer literal 500 is out of range for `int8`"),
            std::string::npos)
      << out;
}

// Unsigned ranges are checked too (300 > 255).
TEST(CompilePipelineTest, UnsignedLiteralOutOfRangeEmitsError) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"let x: uint8 = 300", opts(os));

  EXPECT_NE(out.find("out of range for `uint8`"), std::string::npos) << out;
}

// A float literal past float32's finite range is rejected.
TEST(CompilePipelineTest, FloatLiteralOutOfRangeEmitsError) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"let x: float32 = 1e40", opts(os));

  EXPECT_NE(out.find("out of range for `float32`"), std::string::npos) << out;
}

// An in-range literal is accepted and takes the annotated type.
TEST(CompilePipelineTest, InRangeLiteralAccepted) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"let x: int8 = 100", opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos) << out;
  EXPECT_NE(out.find("LetStmt : int8"), std::string::npos) << out;
}

// Unary minus folds into the literal, so `-128` is range-checked as written
// (in range for int8) rather than as its magnitude 128 (which is not).
TEST(CompilePipelineTest, NegativeLiteralInRangeAccepted) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"let x: int8 = -128", opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos) << out;
  EXPECT_NE(out.find("LetStmt : int8"), std::string::npos) << out;
}

TEST(CompilePipelineTest, NegativeLiteralOutOfRangeEmitsError) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"let x: int8 = -129", opts(os));

  EXPECT_NE(out.find("out of range for `int8`"), std::string::npos) << out;
}

//===----------------------------------------------------------------------===//
// Unary operators — `-`/`+` on a non-literal lower to a unary op call;
// `not` requires a bool.
//===----------------------------------------------------------------------===//

// Negating a binding preserves its type (`-x` where `x : int32` is int32).
TEST(CompilePipelineTest, UnaryNegateBindingPreservesType) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"let x: int32 = 5\nlet y = -x", opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos) << out;
  EXPECT_NE(out.find("CallExpr : int32"), std::string::npos) << out;
}

// `not` on a bool yields bool.
TEST(CompilePipelineTest, UnaryNotProducesBool) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"let b = not true", opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos) << out;
  EXPECT_NE(out.find("LetStmt : bool"), std::string::npos) << out;
}

// `not` on a non-bool is rejected.
TEST(CompilePipelineTest, UnaryNotOnNonBoolEmitsError) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"let b = not 5", opts(os));

  EXPECT_NE(out.find("unary operator `not` cannot be applied"),
            std::string::npos)
      << out;
}

//===----------------------------------------------------------------------===//
// Functions — `fn` lowers to HIR, then the type pass gives it a signature,
// binds params into a function scope, and types the body against them.
//===----------------------------------------------------------------------===//

TEST(CompilePipelineTest, LowersFunctionToHir) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn add(x: int32, y: int32) -> int32 { return x + y }",
                opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos) << out;
  EXPECT_NE(out.find("FuncStmt"), std::string::npos) << out;
  EXPECT_NE(out.find("BlockStmt"), std::string::npos) << out;
  EXPECT_NE(out.find("ReturnStmt"), std::string::npos) << out;
}

// Params resolve their annotations and are visible in the body: `x`/`y` are
// int32, so `x + y` types to int32.
TEST(CompilePipelineTest, TypesParamsAndBody) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn add(x: int32, y: int32) -> int32 { return x + y }",
                opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos) << out;
  EXPECT_NE(out.find("Param : int32"), std::string::npos) << out;
  EXPECT_NE(out.find("IdentExpr : int32"), std::string::npos) << out;
  EXPECT_NE(out.find("CallExpr : int32"), std::string::npos) << out;
}

// A parameter referencing an unknown type name is reported (resolution now
// runs in the type pass, not lowering).
TEST(CompilePipelineTest, UnknownParamTypeEmitsError) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn f(x: bogus) { return }", opts(os));

  EXPECT_NE(out.find("unknown type `bogus`"), std::string::npos) << out;
}

// A use of a parameter outside the function is unresolved — the function
// scope is popped after the body.
TEST(CompilePipelineTest, ParamNotVisibleOutsideFunction) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn f(x: int32) { return x }\nlet y = x", opts(os));

  EXPECT_NE(out.find("unresolved identifier"), std::string::npos) << out;
}

TEST(CompilePipelineTest, LowersBareReturn) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn f() { return }", opts(os));

  EXPECT_NE(out.find("FuncStmt"), std::string::npos) << out;
  EXPECT_NE(out.find("ReturnStmt"), std::string::npos) << out;
}

//===----------------------------------------------------------------------===//
// Forward references — function signatures are hoisted, so a function can
// call a sibling declared later in the same scope.
//===----------------------------------------------------------------------===//

// `caller` refers to `callee`, which is declared below it.
TEST(CompilePipelineTest, ForwardReferenceToLaterFunction) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn caller(x: int32) -> int32 { return callee(x) }\n"
                U"fn callee(x: int32) -> int32 { return x }",
                opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos) << out;
}

// Mutually recursive functions reference each other across declarations.
TEST(CompilePipelineTest, MutuallyRecursiveFunctions) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn ping(n: int32) -> int32 { return pong(n) }\n"
                U"fn pong(n: int32) -> int32 { return ping(n) }",
                opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos) << out;
}

// Hoisting doesn't suppress genuine duplicate-definition errors.
TEST(CompilePipelineTest, DuplicateFunctionNameEmitsError) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn f() { return }\nfn f() { return }", opts(os));

  EXPECT_NE(out.find("`f` is already bound"), std::string::npos) << out;
}

//===----------------------------------------------------------------------===//
// Nested generic functions — a function declared inside another may take its
// own type parameters and capture the enclosing function's. Each `[T]` has a
// distinct identity, so an inner `[U]` is never confused with an outer `[T]`.
//===----------------------------------------------------------------------===//

// An inner function may reference the enclosing type parameter in its body.
TEST(CompilePipelineTest, NestedFunctionCapturesOuterTypeParam) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn outer[T](x: T) -> T {\n"
                U"  fn helper[U](u: U) -> U {\n"
                U"    let z: T = x\n"
                U"    return u\n"
                U"  }\n"
                U"  return x\n"
                U"}",
                opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos) << out;
}

// An inner function's own type parameter is distinct from the outer one:
// returning `U` where the (captured) `T` is expected is a mismatch.
TEST(CompilePipelineTest, NestedTypeParamDistinctFromOuter) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn outer[T](x: T) -> T {\n"
                U"  fn inner[U](y: U) -> T { return y }\n"
                U"  return x\n"
                U"}",
                opts(os));

  EXPECT_NE(out.find("declared to return"), std::string::npos) << out;
}

// A nested function captures an enclosing value binding (a closure). `x` is
// declared in `addOne` and used inside `addTwo`.
TEST(CompilePipelineTest, NestedFunctionCapturesOuterValue) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn addOne(x: int32) {\n"
                U"  fn addTwo() -> int32 { return x + 2 }\n"
                U"  addTwo()\n"
                U"}",
                opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos) << out;
}

// Calling a locally-declared generic function works.
TEST(CompilePipelineTest, CallsLocalGenericFunction) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn outer[T](x: T) -> T {\n"
                U"  fn helper[U](u: U) -> U { return u }\n"
                U"  return helper(x)\n"
                U"}",
                opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos) << out;
}

//===----------------------------------------------------------------------===//
// Return-type checking — a `return expr` must be assignable to the declared
// return type. An untyped literal adapts to it; a widening coercion is OK;
// a mismatch or out-of-range value is an error.
//===----------------------------------------------------------------------===//

// A return value that's already an error (here, an unresolved name) reports
// once — it does not also cascade into a return-type mismatch.
TEST(CompilePipelineTest, PoisonedReturnValueDoesNotCascade) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn f() { return bogus }", opts(os));

  EXPECT_NE(out.find("unresolved identifier"), std::string::npos) << out;
  EXPECT_EQ(out.find("declared to return"), std::string::npos) << out;
}

TEST(CompilePipelineTest, ReturnLiteralAdaptsToReturnType) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn f() -> int8 { return 5 }", opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos) << out;
}

TEST(CompilePipelineTest, ReturnWideningCoerces) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn f() -> float64 { return 5 }", opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos) << out;
}

TEST(CompilePipelineTest, ReturnTypeMismatchEmitsError) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn f() -> int32 { return true }", opts(os));

  EXPECT_NE(out.find("returning `bool` from a function declared to return "
                     "`int32`"),
            std::string::npos)
      << out;
}

TEST(CompilePipelineTest, ReturnOutOfRangeLiteralEmitsError) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn f() -> int8 { return 5000 }", opts(os));

  EXPECT_NE(out.find("out of range for `int8`"), std::string::npos) << out;
}

// A parameter returned at its own type is fine.
TEST(CompilePipelineTest, ReturnParamMatchesType) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn f(x: int32) -> int32 { return x }", opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos) << out;
}

//===----------------------------------------------------------------------===//
// Unit return type — an omitted return annotation defaults to `unit`, and
// `unit` is a resolvable builtin type name. A bare `return` satisfies it; a
// value `return` does not.
//===----------------------------------------------------------------------===//

// A bare `return` in a function with no annotation is fine — the return
// type defaults to `unit`.
TEST(CompilePipelineTest, BareReturnInInferredUnitFunctionIsOk) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn f() { return }", opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos) << out;
}

// `unit` resolves as a builtin type name in a return annotation.
TEST(CompilePipelineTest, ExplicitUnitReturnTypeResolves) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn f() -> unit { return }", opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos) << out;
}

// Returning a value from a function whose return type defaulted to `unit`
// is a mismatch.
TEST(CompilePipelineTest, ValueReturnFromUnitFunctionEmitsError) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn f() { return 5 }", opts(os));

  EXPECT_NE(out.find("returning `int64` from a function declared to return "
                     "`unit`"),
            std::string::npos)
      << out;
}

//===----------------------------------------------------------------------===//
// Function calls — the callee must be a function; args are checked against
// the parameters, and the call's type is the function's return type.
//===----------------------------------------------------------------------===//

// A well-typed call resolves to the function's return type.
TEST(CompilePipelineTest, CallResolvesToReturnType) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn add(x: int32, y: int32) -> int32 { return x + y }\n"
                U"let r = add(1, 2)",
                opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos) << out;
  EXPECT_NE(out.find("LetStmt : int32"), std::string::npos) << out;
  EXPECT_NE(out.find("CallExpr : int32"), std::string::npos) << out;
}

// Too few arguments is an arity error.
TEST(CompilePipelineTest, CallArityMismatchEmitsError) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn add(x: int32, y: int32) -> int32 { return x + y }\n"
                U"let r = add(1)",
                opts(os));

  EXPECT_NE(out.find("expected 2 argument(s), found 1"), std::string::npos)
      << out;
}

// An argument whose type isn't assignable to the parameter is rejected.
TEST(CompilePipelineTest, CallArgTypeMismatchEmitsError) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn id(x: int32) -> int32 { return x }\n"
                U"let r = id(true)",
                opts(os));

  EXPECT_NE(out.find("is not assignable to parameter"), std::string::npos)
      << out;
}

// Calling a non-function value is rejected.
TEST(CompilePipelineTest, CallNonFunctionEmitsError) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"let x = 5\nlet r = x(1)", opts(os));

  EXPECT_NE(out.find("is not callable"), std::string::npos) << out;
}

// An untyped literal argument adapts to the parameter type (no cast needed).
TEST(CompilePipelineTest, CallLiteralArgAdaptsToParam) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn f(x: int8) -> int8 { return x }\nlet r = f(5)", opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos) << out;
}

// An out-of-range literal argument is caught against the parameter type.
TEST(CompilePipelineTest, CallLiteralArgOutOfRangeEmitsError) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn f(x: int8) -> int8 { return x }\nlet r = f(5000)",
                opts(os));

  EXPECT_NE(out.find("out of range for `int8`"), std::string::npos) << out;
}

// An un-annotated `let` from a literal stays open: a later use pins it. So
// `addOne(x)` with x = 5 infers x as int32 rather than defaulting to int64.
TEST(CompilePipelineTest, UnannotatedLetLiteralPinnedByUse) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn addOne(x: int32) -> int32 { return x + 1 }\n"
                U"let x = 5\n"
                U"addOne(x)",
                opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos) << out;
}

//===----------------------------------------------------------------------===//
// Generics — `[T]` parameters are rigid markers in the signature; a call
// site substitutes a fresh inference hole for each and infers it from args.
//===----------------------------------------------------------------------===//

// A generic function declares without error: `: T` resolves to the type
// parameter, not an unknown-type error.
TEST(CompilePipelineTest, GenericFunctionDeclares) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn id[T](x: T) -> T { return x }", opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos) << out;
}

// `T` is only in scope inside its function — referencing it elsewhere is an
// unknown type.
TEST(CompilePipelineTest, TypeParamNotVisibleOutsideFunction) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn id[T](x: T) -> T { return x }\nlet y: T = 5", opts(os));

  EXPECT_NE(out.find("unknown type `T`"), std::string::npos) << out;
}

// `id[T](x: T) -> T` called with an int infers T = int, so the call (and the
// binding) is int64.
TEST(CompilePipelineTest, GenericCallInfersIntReturn) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn id[T](x: T) -> T { return x }\nlet r = id(5)", opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos) << out;
  EXPECT_NE(out.find("LetStmt : int64"), std::string::npos) << out;
  EXPECT_NE(out.find("FuncCallExpr : int64"), std::string::npos) << out;
}

// Called with a bool, the same function infers T = bool.
TEST(CompilePipelineTest, GenericCallInfersBoolReturn) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn id[T](x: T) -> T { return x }\nlet r = id(true)",
                opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos) << out;
  EXPECT_NE(out.find("LetStmt : bool"), std::string::npos) << out;
}

// Two calls with different types must not leak: the int call doesn't pin T
// for the bool call. Both resolve independently.
TEST(CompilePipelineTest, GenericCallsDoNotLeakBetweenSites) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn id[T](x: T) -> T { return x }\n"
                U"let a = id(5)\n"
                U"let b = id(true)",
                opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos) << out;
  EXPECT_NE(out.find("LetStmt : int64"), std::string::npos) << out;
  EXPECT_NE(out.find("LetStmt : bool"), std::string::npos) << out;
}

// A two-parameter generic infers each independently; the return picks the
// second.
TEST(CompilePipelineTest, GenericTwoParamsInferIndependently) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn second[A, B](a: A, b: B) -> B { return b }\n"
                U"let r = second(5, true)",
                opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos) << out;
  EXPECT_NE(out.find("LetStmt : bool"), std::string::npos) << out;
}

// A repeated type parameter must agree across arguments: `pair[T](a: T, b: T)`
// called with mismatched argument types is an error.
TEST(CompilePipelineTest, GenericRepeatedParamMustAgree) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn pair[T](a: T, b: T) -> T { return a }\n"
                U"let r = pair(5, true)",
                opts(os));

  EXPECT_NE(out.find("is not assignable to parameter"), std::string::npos)
      << out;
}

// An annotation pins the inferred type parameter: `id(5)` bound to a `: int8`
// flows int8 into T, so the literal is range-checked as int8.
TEST(CompilePipelineTest, GenericCallResultAdaptsToAnnotation) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"fn id[T](x: T) -> T { return x }\nlet r: int8 = id(5)",
                opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos) << out;
  EXPECT_NE(out.find("LetStmt : int8"), std::string::npos) << out;
}

} // namespace
