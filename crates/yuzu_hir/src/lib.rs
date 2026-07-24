use id_arena::Arena;

mod hir;
mod infer;
mod lowering;
mod printer;
mod source_map;

pub use hir::*;
pub use infer::{InferenceResult, infer};
pub use lowering::lower;
pub use printer::dump;
pub use source_map::HirSourceMap;

#[derive(Default)]
pub struct HirCtx {
    stmts: Arena<Stmt>,
    exprs: Arena<Expr>,
    rels: Arena<Rel>,
    annotations: Arena<TypeAnnotation>,
}

impl HirCtx {
    pub fn new() -> Self {
        Self {
            stmts: Arena::new(),
            exprs: Arena::new(),
            rels: Arena::new(),
            annotations: Arena::new(),
        }
    }

    pub fn alloc_stmt(&mut self, stmt: Stmt) -> StmtId {
        self.stmts.alloc(stmt)
    }

    pub fn stmt(&self, id: StmtId) -> &Stmt {
        &self.stmts[id]
    }

    pub fn alloc_expr(&mut self, expr: Expr) -> ExprId {
        self.exprs.alloc(expr)
    }

    pub fn expr(&self, id: ExprId) -> &Expr {
        &self.exprs[id]
    }

    pub fn alloc_rel(&mut self, rel: Rel) -> RelId {
        self.rels.alloc(rel)
    }

    pub fn rel(&self, id: RelId) -> &Rel {
        &self.rels[id]
    }

    pub fn alloc_annotation(&mut self, annotation: TypeAnnotation) -> TypeAnnotationId {
        self.annotations.alloc(annotation)
    }

    pub fn annotation(&self, id: TypeAnnotationId) -> &TypeAnnotation {
        &self.annotations[id]
    }
}
