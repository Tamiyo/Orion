use id_arena::Id;

use crate::hir::{Ident, Literal, RelId, SymbolId};

pub type ExprId = Id<Expr>;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Op {
    Add,
    Sub,
    Mul,
    Div,
    Pow,
    And,
    Or,
    In,
    NotIn,
    Eq,
    Neq,
    Lt,
    Lte,
    Gt,
    Gte,
    ShiftLeft,
    ShiftRight,
    UnaryPos,
    UnaryNeg,
    UnaryNot,
}

impl Op {
    pub fn symbol(self) -> &'static str {
        match self {
            Op::Add | Op::UnaryPos => "+",
            Op::Sub | Op::UnaryNeg => "-",
            Op::Mul => "*",
            Op::Div => "/",
            Op::Pow => "**",
            Op::And => "and",
            Op::Or => "or",
            Op::In => "in",
            Op::NotIn => "not in",
            Op::Eq => "==",
            Op::Neq => "!=",
            Op::Lt => "<",
            Op::Lte => "<=",
            Op::Gt => ">",
            Op::Gte => ">=",
            Op::ShiftLeft => "<<",
            Op::ShiftRight => ">>",
            Op::UnaryNot => "not",
        }
    }
}

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
    FuncCall {
        callee: ExprId,
        args: Box<[ExprId]>,
    },
    MethodCall {
        receiver: ExprId,
        method: Ident,
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
