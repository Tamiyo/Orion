use id_arena::Id;

pub use yuzu_core::adt::SymbolId;

mod expr;
mod literal;
mod rel;
mod stmt;

pub use expr::{Expr, ExprId, Op, StructFieldInit};
pub use literal::Literal;
pub use rel::{JoinCondition, JoinKind, Rel, RelId, RenameItem, SelectItem, SetItem};
pub use stmt::{FuncParam, Mutability, Stmt, StmtId, StructField};

pub type TypeAnnotationId = Id<TypeAnnotation>;

#[derive(Clone, Copy, PartialEq, Eq)]
pub enum Visibility {
    Public,
    Protected,
    Internal,
    Private,
}

#[derive(Clone, PartialEq, Eq)]
pub enum TypeAnnotation {
    /// The implicit `Self` type of a method receiver (`self`).
    Self_,

    /// A named type, optionally applied to type arguments (`Relation[T]`).
    Named {
        name: Ident,
        args: Box<[TypeAnnotationId]>,
    },

    /// A function type `(params...) -> result`, e.g. `(int32) -> int32`.
    Func {
        params: Box<[TypeAnnotationId]>,
        ret: TypeAnnotationId,
    },

    /// Error-recovery node for input that failed to lower.
    Missing,
}

#[derive(Clone, PartialEq, Eq)]
pub struct TypeBound {
    pub subject: Ident,
    pub traits: Box<[TraitRef]>,
}

#[derive(Clone, PartialEq, Eq)]
pub struct TraitRef {
    pub name: Ident,
}

#[derive(Clone, Copy, PartialEq, Eq)]
pub struct Ident {
    pub symbol: SymbolId,
}

#[derive(Clone, PartialEq, Eq)]
pub struct Root {
    pub stmts: Box<[StmtId]>,
}
