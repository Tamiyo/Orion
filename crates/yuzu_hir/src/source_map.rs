use std::collections::HashMap;

use yuzu_syntax::SyntaxNodePtr;

use crate::{ExprId, RelId, StmtId, TypeAnnotationId};

#[derive(Default)]
pub struct HirSourceMap {
    exprs: HashMap<ExprId, SyntaxNodePtr>,
    rels: HashMap<RelId, SyntaxNodePtr>,
    stmts: HashMap<StmtId, SyntaxNodePtr>,
    annotations: HashMap<TypeAnnotationId, SyntaxNodePtr>,
}

impl HirSourceMap {
    pub fn bind_expr(&mut self, id: ExprId, ptr: SyntaxNodePtr) {
        self.exprs.insert(id, ptr);
    }

    pub fn bind_annotation(&mut self, id: TypeAnnotationId, ptr: SyntaxNodePtr) {
        self.annotations.insert(id, ptr);
    }

    pub fn annotation(&self, id: TypeAnnotationId) -> Option<&SyntaxNodePtr> {
        self.annotations.get(&id)
    }

    pub fn bind_rel(&mut self, id: RelId, ptr: SyntaxNodePtr) {
        self.rels.insert(id, ptr);
    }

    pub fn bind_stmt(&mut self, id: StmtId, ptr: SyntaxNodePtr) {
        self.stmts.insert(id, ptr);
    }

    pub fn expr(&self, id: ExprId) -> Option<&SyntaxNodePtr> {
        self.exprs.get(&id)
    }

    pub fn rel(&self, id: RelId) -> Option<&SyntaxNodePtr> {
        self.rels.get(&id)
    }

    pub fn stmt(&self, id: StmtId) -> Option<&SyntaxNodePtr> {
        self.stmts.get(&id)
    }
}
