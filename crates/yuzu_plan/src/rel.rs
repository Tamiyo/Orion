use id_arena::Id;
use yuzu_core::adt::SymbolId;
use yuzu_types::TypeId;

use crate::expr::ExprId;

pub type RelId = Id<Rel>;

/// A relational operator: the payload of one graph node. What it reads is not
/// stored here — the graph's edges carry the dataflow — so a node is only its
/// own operation, and each carries the `Relation[row]` type it produces.
#[derive(Clone, PartialEq, Eq, Hash)]
pub enum Rel {
    From {
        relation: SymbolId,
        alias: Option<SymbolId>,
        ty: TypeId,
    },
    Join {
        kind: JoinKind,
        condition: Option<JoinCondition>,
        ty: TypeId,
    },
    Select {
        items: Box<[SelectItem]>,
        ty: TypeId,
    },
    Where {
        predicate: ExprId,
        ty: TypeId,
    },
    Distinct {
        ty: TypeId,
    },
    Drop {
        columns: Box<[u32]>,
        ty: TypeId,
    },
    Rename {
        items: Box<[RenameItem]>,
        ty: TypeId,
    },
    Extend {
        items: Box<[SelectItem]>,
        ty: TypeId,
    },
    Set {
        items: Box<[SetItem]>,
        ty: TypeId,
    },
    Limit {
        count: u64,
        offset: Option<u64>,
        ty: TypeId,
    },
    Alias {
        alias: SymbolId,
        ty: TypeId,
    },
}

impl Rel {
    pub fn ty(&self) -> yuzu_types::TypeId {
        match self {
            Rel::From { ty, .. }
            | Rel::Join { ty, .. }
            | Rel::Select { ty, .. }
            | Rel::Where { ty, .. }
            | Rel::Distinct { ty, .. }
            | Rel::Drop { ty, .. }
            | Rel::Rename { ty, .. }
            | Rel::Extend { ty, .. }
            | Rel::Set { ty, .. }
            | Rel::Limit { ty, .. }
            | Rel::Alias { ty, .. } => *ty,
        }
    }

    /// How many relations this operator reads.
    pub fn arity(&self) -> usize {
        match self {
            Rel::From { .. } => 0,
            Rel::Join { .. } => 2,
            Rel::Select { .. }
            | Rel::Where { .. }
            | Rel::Distinct { .. }
            | Rel::Drop { .. }
            | Rel::Rename { .. }
            | Rel::Extend { .. }
            | Rel::Set { .. }
            | Rel::Limit { .. }
            | Rel::Alias { .. } => 1,
        }
    }
}

#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub enum JoinKind {
    Inner,
    Left,
    Right,
    Full,
    Cross,
}

#[derive(Clone, PartialEq, Eq, Hash)]
pub enum JoinCondition {
    On(ExprId),
    Using(Box<[JoinKey]>),
}

/// The pair of columns one `using` key matched. Both sides spell the column the
/// same way, so only their positions tell them apart.
#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub struct JoinKey {
    pub left: u32,
    pub right: u32,
}

#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub struct SelectItem {
    pub body: ExprId,
    pub alias: Option<SymbolId>,
}

#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub struct SetItem {
    pub column: u32,
    pub value: ExprId,
}

#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub struct RenameItem {
    pub column: u32,
    pub to: SymbolId,
}
