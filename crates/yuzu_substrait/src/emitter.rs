use substrait::proto::{Plan, PlanRel, RelRoot, plan_rel};
use substrait::version;
use yuzu_anf::{
    AnfCtx, AnfSourceMap,
    anf::{Expr, ExprId, RelId, Root, Stmt, StmtId},
};
use yuzu_core::adt::StringInterner;
use yuzu_diagnostics::{
    diagnostics::{Span, builder::DiagnosticBuilder, engine::DiagnosticsEngine},
    source_map::SourceId,
};
use yuzu_types::{TypeCtx, TypeId};

use crate::emitter::extensions::Extensions;

mod expr;
mod extensions;
mod rel;
mod types;

/// Emits a plan for the program's query — `None` if it has no query, or if
/// the reduced program contains something a plan cannot express, reported as
/// a diagnostic at its source position.
pub fn emit(
    root: &Root,
    anf: &AnfCtx,
    types: &TypeCtx,
    interner: &StringInterner,
    source_map: &AnfSourceMap,
    diagnostics: &mut DiagnosticsEngine,
    source_id: SourceId,
) -> Option<Plan> {
    let (query, query_stmt) = find_query(root, anf)?;

    let mut emitter = SubstraitEmitter {
        anf,
        types,
        interner,
        source_map,
        diagnostics,
        source_id,
        query_stmt,
        extensions: Extensions::default(),
        row: types.error_ty(),
    };

    // Build the relation first, so function registration populates the tables.
    let relation = emitter.emit_rel(query).ok()?;

    // Output column names come from the query's row type, so aliased,
    // bare-ident, and generated names all carry through.
    let names = emitter
        .row_fields(emitter.rel_ty(query))
        .iter()
        .map(|&(name, _)| interner.text(name).to_string())
        .collect();

    Some(Plan {
        version: Some(version::version_with_producer("yuzu")),
        extension_urns: emitter.extensions.urns(),
        extensions: emitter.extensions.declarations(),
        relations: vec![PlanRel {
            rel_type: Some(plan_rel::RelType::Root(RelRoot {
                input: Some(relation),
                names,
            })),
        }],
        ..Default::default()
    })
}

/// The query is the relational expression statement at the program's tail.
fn find_query(root: &Root, anf: &AnfCtx) -> Option<(RelId, StmtId)> {
    root.stmts.iter().rev().find_map(|&id| match anf.stmt(id) {
        Stmt::Expr { value } => match anf.expr(*value) {
            Expr::Rel(rel) => Some((*rel, id)),
            _ => None,
        },
        _ => None,
    })
}

struct SubstraitEmitter<'e> {
    anf: &'e AnfCtx,
    types: &'e TypeCtx,
    interner: &'e StringInterner,
    source_map: &'e AnfSourceMap,
    diagnostics: &'e mut DiagnosticsEngine,
    source_id: SourceId,
    query_stmt: StmtId,
    extensions: Extensions,
    /// The row struct an expression's column references index into. A stage
    /// sets it before emitting its own expressions; a join's condition sees its
    /// two inputs concatenated, every other stage sees its input's row.
    row: TypeId,
}

/// The reduced program contained something a plan cannot express; a
/// diagnostic has already been reported at its source position.
struct Unsupported;

impl SubstraitEmitter<'_> {
    fn unsupported(&mut self, id: ExprId, message: impl Into<String>) -> Unsupported {
        let range = self
            .source_map
            .expr(id)
            .expect("a reported node is always in the source map")
            .text_range();

        let span = Span {
            source_id: self.source_id,
            range,
        };

        let diagnostic = DiagnosticBuilder::error(span, message)
            .note("expressions must be evaluatable at compile time");

        self.diagnostics.emit(diagnostic);

        Unsupported
    }

    fn unsupported_query(&mut self, message: impl Into<String>) -> Unsupported {
        let range = self
            .source_map
            .stmt(self.query_stmt)
            .expect("a reported node is always in the source map")
            .text_range();

        let span = Span {
            source_id: self.source_id,
            range,
        };

        let diagnostic = DiagnosticBuilder::error(span, message)
            .note("expressions must be evaluatable at compile time");

        self.diagnostics.emit(diagnostic);

        Unsupported
    }
}

#[cfg(test)]
pub(crate) mod test_support {
    use expect_test::Expect;
    use yuzu_ast::ast::{AstNode, Root as AstRoot};
    use yuzu_core::adt::StringInterner;
    use yuzu_diagnostics::{diagnostics::engine::DiagnosticsEngine, source_map::SourceMap};
    use yuzu_hir::HirCtx;
    use yuzu_lexer::lexer::{Lexer, Token};
    use yuzu_types::TypeCtx;

    use crate::to_json;

    pub(crate) const TABLE: &str = "struct Row { a: int32, b: int32 }\ntable t = Row\n";

    fn compile(input: &str) -> (Option<String>, Vec<String>) {
        let mut interner = StringInterner::new();
        let mut diagnostics = DiagnosticsEngine::new();
        let mut sources = SourceMap::new();
        let source_id = sources.add("test".to_string(), input.to_string());

        let tokens: Vec<Token> = Lexer::new(input).collect();
        let syntax = yuzu_parser::parse(&tokens, &mut diagnostics, source_id);
        let ast_root = AstRoot::cast(syntax).expect("root node");

        let mut hir = HirCtx::new();
        let (hir_root, hir_source_map) = yuzu_hir::lower(
            ast_root,
            &mut hir,
            &mut interner,
            &mut diagnostics,
            source_id,
        );

        let mut types = TypeCtx::new();
        let inference = yuzu_hir::infer(
            &hir_root,
            &hir,
            &mut interner,
            &mut types,
            &mut diagnostics,
            &hir_source_map,
            source_id,
        );

        let messages: Vec<&str> = diagnostics
            .diagnostics()
            .iter()
            .map(|d| d.message.as_str())
            .collect();
        assert!(
            messages.is_empty(),
            "program should type-check cleanly, got: {messages:?}"
        );

        let mut anf = yuzu_anf::AnfCtx::new();
        let (anf_root, anf_source_map) = yuzu_anf::lower(
            &hir_root,
            &hir,
            &inference,
            &types,
            &mut anf,
            &mut interner,
            &mut diagnostics,
            &hir_source_map,
            source_id,
        );
        let (reduced, reduced_source_map) =
            yuzu_anf::reduce(&anf_root, &mut anf, &mut interner, &anf_source_map);

        let plan = super::emit(
            &reduced,
            &anf,
            &types,
            &interner,
            &reduced_source_map,
            &mut diagnostics,
            source_id,
        )
        .map(|plan| to_json(&plan));

        let messages: Vec<String> = diagnostics
            .diagnostics()
            .iter()
            .map(|d| d.message.clone())
            .collect();
        (plan, messages)
    }

    pub(crate) fn plan(input: &str) -> Option<String> {
        let (plan, messages) = compile(input);
        assert!(messages.is_empty(), "emission reported: {messages:?}");
        plan
    }

    pub(crate) fn check(input: &str, expected: Expect) {
        expected.assert_eq(&plan(input).expect("program should have a query"));
    }

    pub(crate) fn check_error(input: &str, expected: Expect) {
        let (plan, messages) = compile(input);
        assert!(plan.is_none(), "an unsupported program should have no plan");
        expected.assert_eq(&messages.join("\n"));
    }
}

#[cfg(test)]
mod tests {
    use crate::emitter::test_support::{TABLE, plan};

    #[test]
    fn program_without_query_has_no_plan() {
        assert!(plan(&format!("{TABLE}let x = 1")).is_none());
    }
}
