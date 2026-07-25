use id_arena::Id;

use crate::hir::{ExprId, Ident};

pub type RelId = Id<Rel>;

#[derive(Clone, PartialEq, Eq)]
pub struct SelectItem {
    pub expr: ExprId,
    pub alias: Option<Ident>,
}

#[derive(Clone, PartialEq, Eq)]
pub struct RenameItem {
    pub qualifier: Option<Ident>,
    pub from: Ident,
    pub to: Ident,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum JoinKind {
    Inner,
    Left,
    Right,
    Full,
}

impl JoinKind {
    pub fn keyword(self) -> &'static str {
        match self {
            JoinKind::Inner => "inner",
            JoinKind::Left => "left",
            JoinKind::Right => "right",
            JoinKind::Full => "full",
        }
    }
}

#[derive(Clone, PartialEq, Eq)]
pub enum JoinCondition {
    On(ExprId),
    Using(Box<[Ident]>),
}

#[derive(Clone, PartialEq, Eq)]
pub enum Rel {
    From {
        relation: Ident,
        alias: Option<Ident>,
    },
    Join {
        left: RelId,
        right: RelId,
        kind: JoinKind,
        condition: JoinCondition,
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
