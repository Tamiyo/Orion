use id_arena::Id;
use string_interner::symbol::SymbolU32;

pub type SymbolId = SymbolU32;
pub type TypeId = Id<Type>;

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum Type {
    Int8,
    Int16,
    Int32,
    Int64,
    UInt8,
    UInt16,
    UInt32,
    UInt64,
    Float32,
    Float64,
    Bool,
    String,
    Unit,
    Relation(Relation),
    List(List),
    Struct(Struct),
    Func(Func),
    TypeParam(TypeParam),
    Error,

    // A special type that should not be used outside of type unification.
    TypeVar(TypeVariable),
}

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct Relation {
    pub columns: Vec<Column>,
}

/// One column of a relation's row. A join concatenates rows, so a name alone
/// need not be unique; the qualifier is the relation it can be named through —
/// the `from`/`join` alias it came from, or none once a stage computes it.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct Column {
    pub qualifier: Option<SymbolId>,
    pub name: SymbolId,
    pub ty: TypeId,
}

impl Column {
    pub fn new(qualifier: Option<SymbolId>, name: SymbolId, ty: TypeId) -> Self {
        Self {
            qualifier,
            name,
            ty,
        }
    }

    pub fn named_by(&self, qualifier: SymbolId) -> bool {
        self.qualifier == Some(qualifier)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct List {
    pub inner: TypeId,
}

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct Struct {
    pub name: SymbolId,
    pub fields: Vec<(SymbolId, TypeId)>,
}

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct Func {
    pub args: Vec<TypeId>,
    pub ret_type: TypeId,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct TypeParam {
    pub name: SymbolId,
    pub index: usize,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct TypeVariable {
    pub index: usize,
    pub kind: InferKind,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum InferKind {
    General,
    Int,
    Float,
}

impl Type {
    pub fn is_int(&self) -> bool {
        matches!(
            self,
            Type::Int8
                | Type::Int16
                | Type::Int32
                | Type::Int64
                | Type::UInt8
                | Type::UInt16
                | Type::UInt32
                | Type::UInt64
        )
    }

    pub fn is_float(&self) -> bool {
        matches!(self, Type::Float32 | Type::Float64)
    }

    pub fn is_numeric(&self) -> bool {
        self.is_int() || self.is_float()
    }

    pub fn is_unsigned(&self) -> bool {
        matches!(
            self,
            Type::UInt8 | Type::UInt16 | Type::UInt32 | Type::UInt64
        )
    }

    pub fn is_hole(&self) -> bool {
        matches!(self, Type::TypeVar { .. })
    }
}
