use std::collections::HashMap;

use yuzu_core::adt::StringInterner;
use yuzu_diagnostics::{diagnostics::engine::DiagnosticsEngine, source_map::SourceId};
use yuzu_types::{InferKind, TypeCtx, TypeId, TypeUnifier};

use crate::{ExprId, HirCtx, HirSourceMap, RelId, Root, StmtId};

mod coerce;
mod concretize;
mod inference;
mod op;
mod symbols;

use self::inference::TypeInferrer;

pub fn infer<'i>(
    root: &Root,
    hir: &'i HirCtx,
    interner: &'i mut StringInterner,
    types: &'i mut TypeCtx,
    diagnostics: &'i mut DiagnosticsEngine,
    source_map: &'i HirSourceMap,
    source_id: SourceId,
) -> InferenceResult {
    let mut ctx =
        TypeInferrer::new(hir, types, interner, diagnostics, source_map, source_id).run(root);
    ctx.concretize();
    ctx.finish()
}

#[derive(Default)]
pub struct InferenceResult {
    expr_types: HashMap<ExprId, TypeId>,
    rel_types: HashMap<RelId, TypeId>,
    stmt_types: HashMap<StmtId, TypeId>,
    adjustments: HashMap<ExprId, TypeId>,
}

impl InferenceResult {
    pub fn expr_ty(&self, id: ExprId) -> Option<TypeId> {
        self.expr_types.get(&id).copied()
    }

    pub fn rel_ty(&self, id: RelId) -> Option<TypeId> {
        self.rel_types.get(&id).copied()
    }

    /// The declared type of a declaration statement (struct, table, or func).
    pub fn stmt_ty(&self, id: StmtId) -> Option<TypeId> {
        self.stmt_types.get(&id).copied()
    }

    pub fn adjustment(&self, id: ExprId) -> Option<TypeId> {
        self.adjustments.get(&id).copied()
    }
}

pub(crate) struct InferCtx<'i> {
    types: &'i mut TypeCtx,
    unifier: TypeUnifier,
    result: InferenceResult,
}

impl<'i> InferCtx<'i> {
    fn new(types: &'i mut TypeCtx) -> Self {
        Self {
            types,
            unifier: TypeUnifier::new(),
            result: InferenceResult::default(),
        }
    }

    fn finish(self) -> InferenceResult {
        self.result
    }

    fn fresh_var(&mut self, kind: InferKind) -> TypeId {
        self.unifier.fresh_var(kind, self.types)
    }

    fn unify(&mut self, a: TypeId, b: TypeId) -> bool {
        self.unifier.unify(a, b, self.types)
    }

    fn resolve(&mut self, ty: TypeId) -> TypeId {
        self.unifier.resolve(ty, self.types)
    }

    fn is_numeric(&mut self, id: TypeId) -> bool {
        let id = self.resolve(id);
        let ty = self.types.ty(id);
        ty.is_int() || ty.is_float()
    }

    fn is_int(&mut self, id: TypeId) -> bool {
        let id = self.resolve(id);
        self.types.ty(id).is_int()
    }

    fn expr_ty(&self, id: ExprId) -> Option<TypeId> {
        self.result.expr_ty(id)
    }

    fn bind_expr_ty(&mut self, id: ExprId, ty: TypeId) -> TypeId {
        self.result.expr_types.insert(id, ty);
        ty
    }

    fn bind_rel_ty(&mut self, id: RelId, ty: TypeId) -> TypeId {
        self.result.rel_types.insert(id, ty);
        ty
    }

    fn bind_stmt_ty(&mut self, id: StmtId, ty: TypeId) -> TypeId {
        self.result.stmt_types.insert(id, ty);
        ty
    }
}

#[cfg(test)]
pub(crate) mod test_support {
    use expect_test::Expect;
    use yuzu_core::adt::StringInterner;
    use yuzu_diagnostics::{diagnostics::engine::DiagnosticsEngine, source_map::SourceMap};
    use yuzu_types::TypeCtx;

    use crate::{HirCtx, HirSourceMap, Root};

    pub(crate) fn check(
        build: impl FnOnce(&mut HirCtx, &mut StringInterner) -> Root,
        expected: Expect,
    ) {
        let mut hir = HirCtx::new();
        let mut interner = StringInterner::new();
        let root = build(&mut hir, &mut interner);

        let mut types = TypeCtx::new();
        let mut diagnostics = DiagnosticsEngine::new();
        let source_map = HirSourceMap::default();
        let mut sources = SourceMap::new();
        let source_id = sources.add("test".into(), String::new());

        super::infer(
            &root,
            &hir,
            &mut interner,
            &mut types,
            &mut diagnostics,
            &source_map,
            source_id,
        );

        let rendered = diagnostics
            .diagnostics()
            .iter()
            .map(|d| d.message.as_str())
            .collect::<Vec<_>>()
            .join("\n");
        expected.assert_eq(&rendered);
    }

    pub(crate) fn check_src(input: &str, expected: Expect) {
        use yuzu_ast::ast::{AstNode, Root as AstRoot};
        use yuzu_lexer::lexer::{Lexer, Token};

        let mut hir = HirCtx::new();
        let mut interner = StringInterner::new();
        let mut diagnostics = DiagnosticsEngine::new();
        let mut sources = SourceMap::new();
        let source_id = sources.add("test".into(), input.to_string());

        let tokens: Vec<Token> = Lexer::new(input).collect();
        let syntax = yuzu_parser::parse(&tokens, &mut diagnostics, source_id);
        let ast_root = AstRoot::cast(syntax).expect("root node");

        let (root, source_map) = crate::lower(
            ast_root,
            &mut hir,
            &mut interner,
            &mut diagnostics,
            source_id,
        );

        let mut types = TypeCtx::new();
        super::infer(
            &root,
            &hir,
            &mut interner,
            &mut types,
            &mut diagnostics,
            &source_map,
            source_id,
        );

        let rendered = diagnostics
            .diagnostics()
            .iter()
            .map(|d| d.message.as_str())
            .collect::<Vec<_>>()
            .join("\n");
        expected.assert_eq(&rendered);
    }
}
