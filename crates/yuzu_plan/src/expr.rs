use id_arena::Id;
use yuzu_core::adt::{Float, Int, SymbolId};
use yuzu_types::{AggFunc, TypeId};

pub use yuzu_types::Func;

pub type ExprId = Id<Expr>;

/// A scalar expression over one row.
///
/// A tree, not a statement sequence: every node denotes a value on its own, so
/// an expression folds, compares and evaluates without an environment to
/// resolve names against. Anything that would need one — a call to a user
/// function, a struct value, a query used as a value — is reduced away before
/// a plan exists.
#[derive(Clone, PartialEq, Eq, Hash)]
pub enum Expr {
    /// A column of the input row, by position. A join concatenates rows, so a
    /// name alone would not say which column is meant.
    Column {
        column: u32,
        ty: TypeId,
    },
    Literal {
        value: Const,
        ty: TypeId,
    },
    Call {
        func: Func,
        args: Box<[ExprId]>,
        ty: TypeId,
    },
}

impl Expr {
    pub fn ty(&self) -> TypeId {
        match self {
            Expr::Column { ty, .. } | Expr::Literal { ty, .. } | Expr::Call { ty, .. } => *ty,
        }
    }
}

/// One aggregate computation of an `Aggregate` node. Not an expression: a
/// measure consumes a group, not a row, so it can only appear here, and an
/// expression over its result addresses it as an output column.
#[derive(Clone, PartialEq, Eq, Hash)]
pub struct Measure {
    pub func: AggFunc,
    pub args: Box<[ExprId]>,
    pub ty: TypeId,
}

#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub enum Const {
    Int { value: Int },
    Float { value: Float },
    Bool { value: bool },
    String { value: SymbolId },
}
