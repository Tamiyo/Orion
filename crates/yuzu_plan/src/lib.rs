use std::collections::HashMap;

use id_arena::Arena;

mod expr;
mod graph;
mod printer;
mod rel;

pub use expr::*;
pub use graph::anf_converter::AnfToRelGraphConverter;
pub use graph::{RelGraph, RelGraphConverter};
pub use printer::dump;
pub use rel::*;

/// The arenas a plan's nodes live in. Expressions are interned: structurally
/// equal trees get one id, so equality is an id compare. Relations are
/// deduplicated by the graph, whose intern key pairs a node with its inputs.
#[derive(Default)]
pub struct PlanCtx {
    rels: Arena<Rel>,
    exprs: Arena<Expr>,
    interned_exprs: HashMap<Expr, ExprId>,
}

impl PlanCtx {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn alloc_rel(&mut self, rel: Rel) -> RelId {
        self.rels.alloc(rel)
    }

    pub fn rel(&self, id: RelId) -> &Rel {
        &self.rels[id]
    }

    pub fn rel_count(&self) -> usize {
        self.rels.len()
    }

    pub fn intern_expr(&mut self, expr: Expr) -> ExprId {
        if let Some(&id) = self.interned_exprs.get(&expr) {
            return id;
        }
        let id = self.exprs.alloc(expr.clone());
        self.interned_exprs.insert(expr, id);
        id
    }

    pub fn expr(&self, id: ExprId) -> &Expr {
        &self.exprs[id]
    }

    pub fn expr_count(&self) -> usize {
        self.exprs.len()
    }
}
