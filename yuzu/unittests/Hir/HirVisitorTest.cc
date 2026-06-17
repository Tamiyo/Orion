#include "yuzu/Hir/HirVisitor.h"

#include "yuzu/Diagnostics/DiagnosticsEngine.h"
#include "yuzu/Diagnostics/SourceMap.h"
#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/HirContext.h"

#include <llvm/ADT/ArrayRef.h>

#include <gtest/gtest.h>

#include <array>
#include <string>
#include <vector>

namespace {

using namespace yuzu::hir;
using yuzu::diagnostics::DiagnosticsEngine;
using yuzu::diagnostics::SourceMap;

class HirVisitorTest : public ::testing::Test {
protected:
  SourceMap sources;
  DiagnosticsEngine diagnostics;
  yuzu::util::StringInterner interner;

  /// Hand-built fixture: `Root(stmts = [ExprStmt(IntLit), ExprStmt(IntLit)])`.
  /// All three stmt-level nodes share the same arena via `ctx.getBuilder()`,
  /// matching how the lowerer wires things up in production.
  struct Tree {
    const IntLit *lit1;
    const IntLit *lit2;
    const ExprStmt *stmt1;
    const ExprStmt *stmt2;
    const Root *root;
  };

  Tree build(HirContext &ctx) {
    auto &b = ctx.getBuilder();
    const auto *l1 = b.makeIntLit(1);
    const auto *l2 = b.makeIntLit(2);
    const auto *s1 = b.makeExprStmt(l1);
    const auto *s2 = b.makeExprStmt(l2);
    const std::array<const Stmt *, 2> stmts = {s1, s2};
    const auto *root =
        b.makeRoot(llvm::ArrayRef<const Stmt *>{stmts.data(), stmts.size()});
    return Tree{l1, l2, s1, s2, root};
  }
};

//===----------------------------------------------------------------------===//
// Default traversal: walk children first, then visit (post-order).
//===----------------------------------------------------------------------===//

/// Records every `visitX` callback in the order they fire so the test
/// can assert that the default traverse really is post-order over the
/// schema-declared child edges.
class Recorder : public HirVisitor<Recorder> {
public:
  std::vector<std::string> events;

  void visitIntLit(const IntLit *) { events.push_back("IntLit"); }
  void visitExprStmt(const ExprStmt *) { events.push_back("ExprStmt"); }
  void visitRoot(const Root *) { events.push_back("Root"); }
};

TEST_F(HirVisitorTest, DefaultTraverseIsPostOrder) {
  // Root → ExprStmt → IntLit → (post-order back up). Two stmts means two
  // (IntLit, ExprStmt) pairs before the Root visit fires.
  const auto sourceId = sources.add("<test>", U"");
  HirContext ctx{diagnostics, sourceId, interner};
  const Tree t = build(ctx);

  Recorder r;
  r.visit(t.root);

  const std::vector<std::string> expected = {
      "IntLit", "ExprStmt", "IntLit", "ExprStmt", "Root",
  };
  EXPECT_EQ(r.events, expected);
}

//===----------------------------------------------------------------------===//
// `visitX` participates without touching the walker.
//===----------------------------------------------------------------------===//

class IntCounter : public HirVisitor<IntCounter> {
public:
  int n = 0;
  void visitIntLit(const IntLit *) { ++n; }
};

TEST_F(HirVisitorTest, VisitOverrideCountsLeavesWithoutCustomWalk) {
  const auto sourceId = sources.add("<test>", U"");
  HirContext ctx{diagnostics, sourceId, interner};
  const Tree t = build(ctx);

  IntCounter c;
  c.visit(t.root);

  EXPECT_EQ(c.n, 2);
}

//===----------------------------------------------------------------------===//
// `walkX` override prunes children — the default `traverseX` calls it
// before `visitX`, so an empty walk turns the subtree into a leaf.
//===----------------------------------------------------------------------===//

/// Replaces `walkExprStmt` with a no-op. The visitor still fires
/// `visitExprStmt` (default `traverse` runs walk *then* visit), but the
/// child `IntLit` is never reached because the walker doesn't recurse.
class PruneStmts : public HirVisitor<PruneStmts> {
public:
  int intLits = 0;
  int exprStmts = 0;

  void walkExprStmt(const ExprStmt *) {}
  void visitIntLit(const IntLit *) { ++intLits; }
  void visitExprStmt(const ExprStmt *) { ++exprStmts; }
};

TEST_F(HirVisitorTest, WalkOverrideSkipsChildren) {
  const auto sourceId = sources.add("<test>", U"");
  HirContext ctx{diagnostics, sourceId, interner};
  const Tree t = build(ctx);

  PruneStmts p;
  p.visit(t.root);

  EXPECT_EQ(p.exprStmts, 2);
  EXPECT_EQ(p.intLits, 0);
}

//===----------------------------------------------------------------------===//
// `traverseX` override swaps the whole sequence — here, pre-order.
//===----------------------------------------------------------------------===//

/// Overrides `traverseRoot` to visit before walking. The two stmts are
/// still post-order (their `traverseX` defaults haven't been touched),
/// but the Root visit now fires *before* its children.
class PreOrderRoot : public HirVisitor<PreOrderRoot> {
public:
  std::vector<std::string> events;

  void traverseRoot(const Root *node) {
    visitRoot(node);
    walkRoot(node);
  }

  void visitIntLit(const IntLit *) { events.push_back("IntLit"); }
  void visitExprStmt(const ExprStmt *) { events.push_back("ExprStmt"); }
  void visitRoot(const Root *) { events.push_back("Root"); }
};

TEST_F(HirVisitorTest, TraverseOverrideSwapsOrder) {
  const auto sourceId = sources.add("<test>", U"");
  HirContext ctx{diagnostics, sourceId, interner};
  const Tree t = build(ctx);

  PreOrderRoot p;
  p.visit(t.root);

  const std::vector<std::string> expected = {
      "Root", "IntLit", "ExprStmt", "IntLit", "ExprStmt",
  };
  EXPECT_EQ(p.events, expected);
}

} // namespace
