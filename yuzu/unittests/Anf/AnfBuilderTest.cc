#include "yuzu/Anf/AnfBuilder.h"
#include "yuzu/Anf/AnfVisitor.h"
#include "yuzu/Hir/Ops/BuiltinOps.h"
#include "yuzu/Types/TypeFactory.h"
#include "yuzu/Util/StringInterner.h"

#include <gtest/gtest.h>

namespace {

using namespace yuzu::anf;
using namespace yuzu::types;

// Counts the atoms reached by the visitor's default post-order walk, exercising
// dispatch into the generated `visitX` hooks.
class AtomCounter : public AnfVisitor<AtomCounter> {
public:
  int varRefs = 0;
  int intConsts = 0;
  void visitVarRef(const VarRef *) { ++varRefs; }
  void visitIntConst(const IntConst *) { ++intConsts; }
};

// Builds the ANF for roughly `from employees e |> select e.id as x`, with a
// `let t = 1 + 2` bind to exercise the scalar layer (Constant / PrimOp /
// LetBind, reusing HIR's operator descriptors), then walks it.
TEST(AnfBuilderTest, BuildsAndWalksProjection) {
  yuzu::util::StringInterner interner;
  TypeFactory factory{interner};
  AnfBuilder builder;

  const Type *i32 = factory.getInt32Type();
  const Type *rowRel = factory.getRelationType(i32);

  const Scan *scan = builder.makeScan(builder.makeIdent(U"employees"), rowRel);

  // let t = 1 + 2
  const Atom *one = builder.makeIntConst(1, i32);
  const Atom *two = builder.makeIntConst(2, i32);
  const PrimOp *add =
      builder.makePrimOp(yuzu::hir::AddOp::get(), {one, two}, i32);
  const LetBind *bind = builder.makeLetBind(builder.makeIdent(U"t"), add);

  // e.id  ->  FieldRef(VarRef(e), id)
  const VarRef *row = builder.makeVarRef(builder.makeIdent(U"e"), i32);
  const FieldRef *eId =
      builder.makeFieldRef(row, builder.makeIdent(U"id"), i32);
  const Column *col = builder.makeColumn(builder.makeIdent(U"x"), eId);

  const Project *project = builder.makeProject(scan, {bind}, {col}, rowRel);
  const Root *root = builder.makeRoot(project);

  // Tree shape + on-node types.
  ASSERT_NE(root, nullptr);
  EXPECT_EQ(root->getPlan(), project);
  EXPECT_EQ(project->getInput(), scan);
  ASSERT_EQ(project->getBinds().size(), 1u);
  ASSERT_EQ(project->getColumns().size(), 1u);
  EXPECT_EQ(eId->getType(), i32);
  EXPECT_EQ(add->getOp(), yuzu::hir::AddOp::get());

  // The walk reaches the single VarRef and both IntConsts.
  AtomCounter counter;
  counter.visit(root);
  EXPECT_EQ(counter.varRefs, 1);
  EXPECT_EQ(counter.intConsts, 2);
}

} // namespace
