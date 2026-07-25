use yuzu_core::adt::{Float, Int};

use crate::hir::SymbolId;

#[derive(Clone, Copy, PartialEq, Eq)]
pub enum Literal {
    Bool { value: bool },
    Int { value: Int },
    Float { value: Float },
    String { value: SymbolId },
    Missing,
}
