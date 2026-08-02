use std::collections::HashMap;

use yuzu_syntax::SyntaxNodePtr;

use crate::{AtomId, ExprId, StmtId};

#[derive(Default)]
pub struct AnfSourceMap {
    stmts: HashMap<StmtId, SyntaxNodePtr>,
    exprs: HashMap<ExprId, SyntaxNodePtr>,
    atoms: HashMap<AtomId, SyntaxNodePtr>,
}

impl AnfSourceMap {
    pub fn bind_stmt(&mut self, id: StmtId, ptr: SyntaxNodePtr) {
        self.stmts.insert(id, ptr);
    }

    pub fn bind_expr(&mut self, id: ExprId, ptr: SyntaxNodePtr) {
        self.exprs.insert(id, ptr);
    }

    pub fn bind_atom(&mut self, id: AtomId, ptr: SyntaxNodePtr) {
        self.atoms.insert(id, ptr);
    }

    pub fn stmt(&self, id: StmtId) -> Option<&SyntaxNodePtr> {
        self.stmts.get(&id)
    }

    pub fn expr(&self, id: ExprId) -> Option<&SyntaxNodePtr> {
        self.exprs.get(&id)
    }

    pub fn atom(&self, id: AtomId) -> Option<&SyntaxNodePtr> {
        self.atoms.get(&id)
    }
}
