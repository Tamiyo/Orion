// A Variant aggregates its concrete children as `std::variant<...>`. canCast
// is a single ||-chain; cast dispatches to each concrete child in turn.
//
// RUN: %yuzu-tblgen -gen-ast-node-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def TestRoot : Base {
  let Namespace = "test::ast";
}

def TestExpr : Variant<TestRoot>;
def TestAlpha : Node<TestExpr>;
def TestBeta  : Node<TestExpr>;

// CHECK:       class TestExpr;

// CHECK-LABEL: class TestExpr : public std::variant<TestAlpha, TestBeta>
// CHECK:       using std::variant<TestAlpha, TestBeta>::variant;
// CHECK:       TestExpr() = delete;
// CHECK:       static bool canCast(SyntaxKind kind)
// CHECK-SAME:  TestAlpha::canCast(kind) || TestBeta::canCast(kind)
// CHECK:       static std::optional<TestExpr> cast(syntax::SyntaxNode node)
// CHECK:       if (TestAlpha::canCast(kind))
// CHECK:       return std::make_optional<TestExpr>(TestAlpha(std::move(node)));
// CHECK:       if (TestBeta::canCast(kind))
// CHECK:       return std::make_optional<TestExpr>(TestBeta(std::move(node)));
