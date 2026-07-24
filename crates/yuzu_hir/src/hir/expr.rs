use id_arena::Id;

use crate::{
    hir::{Ident, SymbolId},
    Literal, Op, RelId,
};

pub type ExprId = Id<Expr>;

#[derive(Clone, Copy, PartialEq, Eq)]
pub struct StructFieldInit {
    pub name: Ident,
    pub value: ExprId,
}

#[derive(Clone, PartialEq, Eq)]
pub enum Expr {
    Ident {
        value: Ident,
    },
    Call {
        op: Op,
        args: Box<[ExprId]>,
    },
    MethodCall {
        callee: ExprId,
        args: Box<[ExprId]>,
    },
    FieldAccess {
        base: ExprId,
        field: Ident,
    },
    StructInit {
        name: SymbolId,
        fields: Box<[StructFieldInit]>,
    },
    ListInit {
        elements: Box<[ExprId]>,
    },
    Literal(Literal),
    Rel(RelId),

    /// Error-recovery node for input that failed to lower.
    Missing,
}
