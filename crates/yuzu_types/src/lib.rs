use std::collections::HashMap;

use id_arena::Arena;

mod types;
mod unifier;
pub use types::*;
pub use unifier::*;

pub struct TypeCtx {
    types: Arena<Type>,
    interned: HashMap<Type, TypeId>,

    error: TypeId,
    bool: TypeId,
    str: TypeId,
    unit: TypeId,
    int64: TypeId,
    float64: TypeId,
}

impl TypeCtx {
    pub fn new() -> Self {
        let mut types = Arena::new();
        let mut interned = HashMap::new();

        fn intern(
            types: &mut Arena<Type>,
            interned: &mut HashMap<Type, TypeId>,
            ty: Type,
        ) -> TypeId {
            if let Some(&id) = interned.get(&ty) {
                return id;
            }
            let id = types.alloc(ty.clone());
            interned.insert(ty, id);
            id
        }

        let error = intern(&mut types, &mut interned, Type::Error);
        let bool = intern(&mut types, &mut interned, Type::Bool);
        let str = intern(&mut types, &mut interned, Type::String);
        let unit = intern(&mut types, &mut interned, Type::Unit);
        let int64 = intern(&mut types, &mut interned, Type::Int64);
        let float64 = intern(&mut types, &mut interned, Type::Float64);

        Self {
            types,
            interned,
            error,
            bool,
            str,
            unit,
            int64,
            float64,
        }
    }

    pub fn intern_ty(&mut self, ty: Type) -> TypeId {
        if let Some(&id) = self.interned.get(&ty) {
            return id;
        }
        let id = self.types.alloc(ty.clone());
        self.interned.insert(ty, id);
        id
    }

    pub fn ty(&self, id: TypeId) -> &Type {
        &self.types[id]
    }

    pub fn error_ty(&self) -> TypeId {
        self.error
    }

    pub fn bool_ty(&self) -> TypeId {
        self.bool
    }

    pub fn str_ty(&self) -> TypeId {
        self.str
    }

    pub fn unit_ty(&self) -> TypeId {
        self.unit
    }

    pub fn int64_ty(&self) -> TypeId {
        self.int64
    }

    pub fn float64_ty(&self) -> TypeId {
        self.float64
    }

    pub fn relation_ty(&mut self, columns: Vec<Column>) -> TypeId {
        self.intern_ty(Type::Relation(Relation { columns }))
    }

    /// A relation over a declared row type, every column named through the one
    /// qualifier the source is addressed by.
    pub fn relation_of_row(&mut self, row: TypeId, qualifier: Option<SymbolId>) -> TypeId {
        let Type::Struct(row) = self.ty(row) else {
            return self.error_ty();
        };
        let columns = row
            .fields
            .clone()
            .into_iter()
            .map(|(name, ty)| Column::new(qualifier, name, ty))
            .collect();
        self.relation_ty(columns)
    }

    pub fn list_ty(&mut self, inner: TypeId) -> TypeId {
        self.intern_ty(Type::List(List { inner }))
    }

    pub fn struct_ty(&mut self, name: SymbolId, fields: Vec<(SymbolId, TypeId)>) -> TypeId {
        self.intern_ty(Type::Struct(Struct { name, fields }))
    }

    pub fn func_ty(&mut self, args: Vec<TypeId>, ret_type: TypeId) -> TypeId {
        self.intern_ty(Type::Func(FuncType { args, ret_type }))
    }

    pub fn type_param_ty(&mut self, name: SymbolId, index: usize) -> TypeId {
        self.intern_ty(Type::TypeParam(TypeParam { name, index }))
    }
}

impl Default for TypeCtx {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use string_interner::Symbol;

    fn sym(n: usize) -> SymbolId {
        SymbolId::try_from_usize(n).unwrap()
    }

    #[test]
    fn constants_are_cached_and_match_interning() {
        let mut types = TypeCtx::new();
        assert_eq!(types.error_ty(), types.intern_ty(Type::Error));
        assert_eq!(types.bool_ty(), types.intern_ty(Type::Bool));
        assert_eq!(types.str_ty(), types.intern_ty(Type::String));
        assert_eq!(types.unit_ty(), types.intern_ty(Type::Unit));
        assert_eq!(types.int64_ty(), types.intern_ty(Type::Int64));
        assert_eq!(types.float64_ty(), types.intern_ty(Type::Float64));
    }

    #[test]
    fn relation_ty_is_hash_consed() {
        let mut types = TypeCtx::new();
        let int = types.int64_ty();
        let columns = vec![Column::new(Some(sym(0)), sym(1), int)];
        let a = types.relation_ty(columns.clone());
        let b = types.relation_ty(columns.clone());
        assert_eq!(a, b);
        assert_eq!(types.ty(a), &Type::Relation(Relation { columns }));
    }

    #[test]
    fn relations_differing_only_by_qualifier_are_distinct() {
        let mut types = TypeCtx::new();
        let int = types.int64_ty();
        let a = types.relation_ty(vec![Column::new(Some(sym(0)), sym(2), int)]);
        let b = types.relation_ty(vec![Column::new(Some(sym(1)), sym(2), int)]);
        assert_ne!(a, b);
    }

    #[test]
    fn relation_of_row_qualifies_every_column() {
        let mut types = TypeCtx::new();
        let int = types.int64_ty();
        let row = types.struct_ty(sym(0), vec![(sym(1), int), (sym(2), int)]);
        let relation = types.relation_of_row(row, Some(sym(3)));
        let Type::Relation(relation) = types.ty(relation) else {
            panic!("expected a relation")
        };
        assert!(
            relation
                .columns
                .iter()
                .all(|column| column.named_by(sym(3)))
        );
        assert_eq!(
            relation.columns.iter().map(|c| c.name).collect::<Vec<_>>(),
            vec![sym(1), sym(2)]
        );
    }

    #[test]
    fn list_ty_is_hash_consed() {
        let mut types = TypeCtx::new();
        let inner = types.bool_ty();
        let a = types.list_ty(inner);
        let b = types.list_ty(inner);
        assert_eq!(a, b);
        assert_eq!(types.ty(a), &Type::List(List { inner }));
    }

    #[test]
    fn struct_ty_is_hash_consed() {
        let mut types = TypeCtx::new();
        let int = types.int64_ty();
        let fields = vec![(sym(0), int)];
        let a = types.struct_ty(sym(1), fields.clone());
        let b = types.struct_ty(sym(1), fields.clone());
        assert_eq!(a, b);
        assert_eq!(
            types.ty(a),
            &Type::Struct(Struct {
                name: sym(1),
                fields
            })
        );
    }

    #[test]
    fn func_ty_is_hash_consed() {
        let mut types = TypeCtx::new();
        let int = types.int64_ty();
        let boolean = types.bool_ty();
        let a = types.func_ty(vec![int], boolean);
        let b = types.func_ty(vec![int], boolean);
        assert_eq!(a, b);
        assert_eq!(
            types.ty(a),
            &Type::Func(FuncType {
                args: vec![int],
                ret_type: boolean,
            })
        );
    }

    #[test]
    fn type_param_ty_is_hash_consed() {
        let mut types = TypeCtx::new();
        let a = types.type_param_ty(sym(0), 0);
        let b = types.type_param_ty(sym(0), 0);
        assert_eq!(a, b);
        assert_eq!(
            types.ty(a),
            &Type::TypeParam(TypeParam {
                name: sym(0),
                index: 0,
            })
        );
    }

    #[test]
    fn distinct_inner_types_give_distinct_constructions() {
        let mut types = TypeCtx::new();
        let int = types.int64_ty();
        let boolean = types.bool_ty();
        let int_list = types.list_ty(int);
        let bool_list = types.list_ty(boolean);
        assert_ne!(int_list, bool_list);
    }
}
