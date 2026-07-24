use crate::anf::{AtomId, Expr, StmtId};

pub(crate) enum Term {
    Stmt(StmtId),
    Expr(Expr),
    Atom(AtomId),
    Unit,
    Yield(Option<AtomId>),
}
