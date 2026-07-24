use yuzu_types::{SymbolId, TypeId};

use crate::anf::{AtomId, ExprId, StructFieldInit};

pub(crate) enum Value {
    Atom(AtomId),
    Struct {
        name: SymbolId,
        fields: Box<[StructFieldInit]>,
        ty: TypeId,
        origin: ExprId,
    },
    List {
        elements: Box<[AtomId]>,
        ty: TypeId,
        origin: ExprId,
    },
}
