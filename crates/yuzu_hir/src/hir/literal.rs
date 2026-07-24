use crate::hir::SymbolId;

#[derive(Clone, Copy, PartialEq, Eq)]
pub enum Literal {
    Bool { value: bool },
    Int { value: u64 },
    Float { value: u64 },
    String { value: SymbolId },
    Missing,
}
