#include "yuzu/Substrait/SubstraitEmitter.h"

#include "yuzu/Anf/AnfContext.h"
#include "yuzu/Anf/AnfLowerer.h"
#include "yuzu/Anf/Reduction/AnfReducer.h"
#include "yuzu/Ast/Ast.h"
#include "yuzu/Diagnostics/DiagnosticsEngine.h"
#include "yuzu/Diagnostics/SourceMap.h"
#include "yuzu/Hir/HirContext.h"
#include "yuzu/Hir/HirLowerer.h"
#include "yuzu/Hir/Types/TypeResolver.h"
#include "yuzu/Lexer/Lexer.h"
#include "yuzu/Parser/Grammar/Grammar.h"
#include "yuzu/Parser/Parser.h"
#include "yuzu/Parser/TokenSink.h"
#include "yuzu/Parser/TokenSource.h"

#include <llvm/Support/JSON.h>

#include <gtest/gtest.h>

#include <string>

namespace {

using namespace yuzu;

// Compiles a query to reduced ANF and emits a Substrait plan, for asserting on
// the plan's JSON.
class SubstraitEmitterTest : public ::testing::Test {
protected:
  diagnostics::SourceMap sources;
  diagnostics::DiagnosticsEngine diagnostics;
  diagnostics::SourceId sourceId = sources.add("<test>", U"");
  util::StringInterner interner;
  hir::HirContext hirCtx{diagnostics, sourceId, interner};

  std::string emit(std::u32string_view source) {
    auto tokens = lexer::Lexer(source).getTokens();
    auto parser = parser::Parser(parser::TokenSource(tokens));
    parser::parseRoot(parser);
    auto events = std::move(parser).finish();
    auto sink = parser::TokenSink(std::move(tokens), std::move(events),
                                  diagnostics, sourceId);
    const auto syntaxRoot = ast::SyntaxNode::createRoot(sink.finish().green);
    const hir::Root *hirRoot = hir::HirLowerer(hirCtx, diagnostics, sourceId)
                                   .lower(ast::Root{syntaxRoot});
    hir::TypeResolver(hirCtx).resolve(hirRoot);

    anf::AnfContext anfCtx{diagnostics, sourceId, hirCtx, interner};
    const anf::Root *program =
        anf::AnfLowerer{anfCtx, diagnostics, sourceId}.lowerRoot(hirRoot);
    anf::AnfReducer(anfCtx).reduce(program);
    EXPECT_TRUE(diagnostics.getDiagnostics().empty());

    return substrait::SubstraitEmitter().emit(program);
  }
};

// A `from … |> select <expr>` query emits a Read under a Project, with the
// column lowered to a scalar function over a field selection and a literal.
TEST_F(SubstraitEmitterTest, EmitsReadProjectAndScalarFunction) {
  const std::string plan = emit(UR"(
    struct Employee { id: int32, tenure: int32 }
    table employees = Employee
    from employees e |> select e.tenure + 7 as bonus
  )");

  auto parsed = llvm::json::parse(plan);
  ASSERT_TRUE(static_cast<bool>(parsed)) << "plan is not valid JSON";
  const llvm::json::Object *root = parsed->getAsObject();
  ASSERT_NE(root, nullptr);

  // relations[0].root
  const llvm::json::Array *relations = root->getArray("relations");
  ASSERT_NE(relations, nullptr);
  ASSERT_EQ(relations->size(), 1u);
  const llvm::json::Object *rootRel =
      (*relations)[0].getAsObject()->getObject("root");
  ASSERT_NE(rootRel, nullptr);

  // Output column name is the alias.
  const llvm::json::Array *names = rootRel->getArray("names");
  ASSERT_NE(names, nullptr);
  ASSERT_EQ(names->size(), 1u);
  EXPECT_EQ((*names)[0].getAsString(), "bonus");

  // root.input.project.input.read.namedTable.names == ["employees"]
  const llvm::json::Object *project =
      rootRel->getObject("input")->getObject("project");
  ASSERT_NE(project, nullptr);
  const llvm::json::Object *read =
      project->getObject("input")->getObject("read");
  ASSERT_NE(read, nullptr);
  const llvm::json::Array *tableNames =
      read->getObject("namedTable")->getArray("names");
  ASSERT_NE(tableNames, nullptr);
  EXPECT_EQ((*tableNames)[0].getAsString(), "employees");

  // The single projected expression is a scalar function (the add).
  const llvm::json::Array *expressions = project->getArray("expressions");
  ASSERT_NE(expressions, nullptr);
  ASSERT_EQ(expressions->size(), 1u);
  EXPECT_NE((*expressions)[0].getAsObject()->getObject("scalarFunction"),
            nullptr);

  // The `add` function is declared in the extensions.
  const llvm::json::Array *extensions = root->getArray("extensions");
  ASSERT_NE(extensions, nullptr);
  bool sawAdd = false;
  for (const llvm::json::Value &ext : *extensions) {
    const auto name =
        ext.getAsObject()->getObject("extensionFunction")->getString("name");
    if (name && name->starts_with("add")) {
      sawAdd = true;
    }
  }
  EXPECT_TRUE(sawAdd);
}

// A second `select` referencing an earlier stage's column emits nested
// projects: the outer projects a `selection` of the column the inner computed.
TEST_F(SubstraitEmitterTest, EmitsChainedColumnReference) {
  const std::string plan = emit(UR"(
    struct Employee { id: int32, tenure: int32 }
    table employees = Employee
    from employees e |> select e.tenure + 7 as bonus |> select bonus
  )");

  auto parsed = llvm::json::parse(plan);
  ASSERT_TRUE(static_cast<bool>(parsed)) << "plan is not valid JSON";
  const llvm::json::Object *root =
      (*parsed->getAsObject()->getArray("relations"))[0]
          .getAsObject()
          ->getObject("root");
  ASSERT_NE(root, nullptr);

  // The output column keeps its name across the stage.
  const llvm::json::Array *names = root->getArray("names");
  ASSERT_NE(names, nullptr);
  ASSERT_EQ(names->size(), 1u);
  EXPECT_EQ((*names)[0].getAsString(), "bonus");

  // Outer project: its single expression is a field selection...
  const llvm::json::Object *outer =
      root->getObject("input")->getObject("project");
  ASSERT_NE(outer, nullptr);
  const llvm::json::Array *outerExprs = outer->getArray("expressions");
  ASSERT_NE(outerExprs, nullptr);
  ASSERT_EQ(outerExprs->size(), 1u);
  EXPECT_NE((*outerExprs)[0].getAsObject()->getObject("selection"), nullptr);

  // ...over an inner project (the `bonus` computation), over the read.
  const llvm::json::Object *inner =
      outer->getObject("input")->getObject("project");
  ASSERT_NE(inner, nullptr);
  EXPECT_NE((*inner->getArray("expressions"))[0].getAsObject()->getObject(
                "scalarFunction"),
            nullptr);
  EXPECT_NE(inner->getObject("input")->getObject("read"), nullptr);
}

// A `|> where` stage emits a FilterRel over the input, with the predicate as
// the condition; the output column names are preserved (filter keeps the row).
TEST_F(SubstraitEmitterTest, EmitsWhereAsFilter) {
  const std::string plan = emit(UR"(
    struct Employee { id: int32, tenure: int32 }
    table employees = Employee
    from employees e |> select e.tenure as t |> where t > 10
  )");

  auto parsed = llvm::json::parse(plan);
  ASSERT_TRUE(static_cast<bool>(parsed)) << "plan is not valid JSON";
  const llvm::json::Object *root =
      (*parsed->getAsObject()->getArray("relations"))[0]
          .getAsObject()
          ->getObject("root");
  ASSERT_NE(root, nullptr);

  // The filter preserves the input's columns.
  const llvm::json::Array *names = root->getArray("names");
  ASSERT_NE(names, nullptr);
  ASSERT_EQ(names->size(), 1u);
  EXPECT_EQ((*names)[0].getAsString(), "t");

  // Top rel is a filter: a scalar-function condition over a project input.
  const llvm::json::Object *filter =
      root->getObject("input")->getObject("filter");
  ASSERT_NE(filter, nullptr);
  EXPECT_NE(filter->getObject("condition")->getObject("scalarFunction"),
            nullptr);
  EXPECT_NE(filter->getObject("input")->getObject("project"), nullptr);
}

// A `|> distinct` stage emits an AggregateRel grouping by every column with no
// measures, over the input; the output columns are preserved.
TEST_F(SubstraitEmitterTest, EmitsDistinctAsAggregate) {
  const std::string plan = emit(UR"(
    struct Employee { id: int32, tenure: int32 }
    table employees = Employee
    from employees e |> select e.id as id, e.tenure as t |> distinct
  )");

  auto parsed = llvm::json::parse(plan);
  ASSERT_TRUE(static_cast<bool>(parsed)) << "plan is not valid JSON";
  const llvm::json::Object *root =
      (*parsed->getAsObject()->getArray("relations"))[0]
          .getAsObject()
          ->getObject("root");
  ASSERT_NE(root, nullptr);

  // Both columns carry through.
  const llvm::json::Array *names = root->getArray("names");
  ASSERT_NE(names, nullptr);
  ASSERT_EQ(names->size(), 2u);

  // Top rel is an aggregate: one grouping over every column, no measures.
  const llvm::json::Object *aggregate =
      root->getObject("input")->getObject("aggregate");
  ASSERT_NE(aggregate, nullptr);
  const llvm::json::Array *groupings = aggregate->getArray("groupings");
  ASSERT_NE(groupings, nullptr);
  ASSERT_EQ(groupings->size(), 1u);
  const llvm::json::Array *groupExprs =
      (*groupings)[0].getAsObject()->getArray("groupingExpressions");
  ASSERT_NE(groupExprs, nullptr);
  EXPECT_EQ(groupExprs->size(), 2u);
  EXPECT_NE(aggregate->getObject("input")->getObject("project"), nullptr);
}

// No query in the program means no plan.
TEST_F(SubstraitEmitterTest, NoQueryEmitsEmpty) {
  EXPECT_TRUE(emit(UR"(
    fn f(x: int32) -> int32 { return x + 1 }
  )")
                  .empty());
}

} // namespace
