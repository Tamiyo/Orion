use id_arena::Id;
use yuzu_core::adt::{Float, Int};

pub use yuzu_core::adt::SymbolId;

pub type StmtId = Id<Stmt>;
pub type ExprId = Id<Expr>;
pub type RelId = Id<Rel>;
pub type TypeAnnotationId = Id<TypeAnnotation>;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Mutability {
    Immutable,
    Mutable,
}

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
pub struct Ident {
    pub symbol: SymbolId,
}

#[derive(Clone, PartialEq, Eq)]
pub struct Root {
    pub stmts: Box<[StmtId]>,
}

#[derive(Clone, PartialEq, Eq)]
pub struct StructField {
    pub name: Ident,
    pub mutability: Mutability,
    pub type_annotation: TypeAnnotationId,
}

#[derive(Clone, PartialEq, Eq)]
pub struct FuncParam {
    pub name: Ident,
    pub type_annotation: TypeAnnotationId,
}

#[derive(Clone, PartialEq, Eq)]
pub enum Stmt {
    Struct {
        name: Ident,
        fields: Box<[StructField]>,
    },
    Impl {
        trait_ref: Option<TraitRef>,
        name: Ident,
        methods: Box<[StmtId]>,
    },
    Trait {
        name: Ident,
        methods: Box<[StmtId]>,
    },
    Func {
        name: Ident,
        type_params: Box<[Ident]>,
        params: Box<[FuncParam]>,
        type_bounds: Box<[TypeBound]>,
        ret_type_annotation: TypeAnnotationId,
        body: Option<StmtId>,
    },
    Block {
        stmts: Box<[StmtId]>,
    },
    Table {
        name: Ident,
        row: Ident,
    },
    InlineTable {
        name: Ident,
        fields: Box<[StructField]>,
    },
    Let {
        name: Ident,
        mutability: Mutability,
        type_annotation: Option<TypeAnnotationId>,
        expr: ExprId,
    },
    Assign {
        target: ExprId,
        value: ExprId,
    },
    Return {
        expr: Option<ExprId>,
    },
    Expr {
        expr: ExprId,
    },

    /// Error-recovery node for input that failed to lower.
    Missing,
}

#[derive(Clone, PartialEq, Eq)]
pub struct SelectItem {
    pub expr: ExprId,
    pub alias: Option<Ident>,
}

#[derive(Clone, PartialEq, Eq)]
pub struct RenameItem {
    pub from: Ident,
    pub to: Ident,
}

#[derive(Clone, PartialEq, Eq)]
pub enum Rel {
    From {
        relation: Ident,
        alias: Option<Ident>,
    },
    Select {
        input: RelId,
        items: Box<[SelectItem]>,
    },
    Where {
        input: RelId,
        predicate: ExprId,
    },
    Distinct {
        input: RelId,
    },
    Drop {
        input: RelId,
        items: Box<[Ident]>,
    },
    Rename {
        input: RelId,
        items: Box<[RenameItem]>,
    },
    Extend {
        input: RelId,
        items: Box<[SelectItem]>,
    },

    /// Error-recovery node for input that failed to lower.
    Missing,
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

#[derive(Clone, Copy, PartialEq, Eq)]
pub enum Literal {
    Bool { value: bool },
    Int { value: Int },
    Float { value: Float },
    String { value: SymbolId },
    Missing,
}
