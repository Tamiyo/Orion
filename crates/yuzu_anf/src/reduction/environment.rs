use yuzu_types::{SymbolId, TypeId};

use crate::{
    anf::{AtomId, BindingId, ExprId, StructFieldInit},
    reduction::value::Value,
};

/// Binding values, stored densely: `BindingId` is an arena index, so a slot
/// vector replaces hashing on the reducer's hottest path.
#[derive(Default)]
pub(crate) struct Environment {
    values: Vec<Option<Value>>,
}

impl Environment {
    fn get(&self, binding: BindingId) -> Option<&Value> {
        self.values.get(binding.index())?.as_ref()
    }

    fn set(&mut self, binding: BindingId, value: Value) {
        let index = binding.index();
        if index >= self.values.len() {
            self.values.resize_with(index + 1, || None);
        }
        self.values[index] = Some(value);
    }

    pub(crate) fn bind_atom(&mut self, binding: BindingId, atom: AtomId) {
        self.set(binding, Value::Atom(atom));
    }

    pub(crate) fn atom(&self, binding: BindingId) -> Option<AtomId> {
        match self.get(binding) {
            Some(Value::Atom(atom)) => Some(*atom),
            _ => None,
        }
    }

    pub(crate) fn bind_struct(
        &mut self,
        binding: BindingId,
        name: SymbolId,
        fields: Box<[StructFieldInit]>,
        ty: TypeId,
        origin: ExprId,
    ) {
        self.set(
            binding,
            Value::Struct {
                name,
                fields,
                ty,
                origin,
            },
        );
    }

    pub(crate) fn struct_(
        &self,
        binding: BindingId,
    ) -> Option<(SymbolId, Box<[StructFieldInit]>, TypeId, ExprId)> {
        match self.get(binding) {
            Some(Value::Struct {
                name,
                fields,
                ty,
                origin,
            }) => Some((*name, fields.clone(), *ty, *origin)),
            _ => None,
        }
    }

    pub(crate) fn bind_list(
        &mut self,
        binding: BindingId,
        elements: Box<[AtomId]>,
        ty: TypeId,
        origin: ExprId,
    ) {
        self.set(
            binding,
            Value::List {
                elements,
                ty,
                origin,
            },
        );
    }

    pub(crate) fn list(&self, binding: BindingId) -> Option<(Box<[AtomId]>, TypeId, ExprId)> {
        match self.get(binding) {
            Some(Value::List {
                elements,
                ty,
                origin,
            }) => Some((elements.clone(), *ty, *origin)),
            _ => None,
        }
    }

    pub(crate) fn list_elements(&self, binding: BindingId) -> Option<&[AtomId]> {
        match self.get(binding) {
            Some(Value::List { elements, .. }) => Some(elements),
            _ => None,
        }
    }

    pub(crate) fn is_aggregate_value(&self, binding: BindingId) -> bool {
        matches!(
            self.get(binding),
            Some(Value::Struct { .. } | Value::List { .. })
        )
    }

    pub(crate) fn struct_fields(&self, binding: BindingId) -> Option<&[StructFieldInit]> {
        match self.get(binding) {
            Some(Value::Struct { fields, .. }) => Some(fields),
            _ => None,
        }
    }

    pub(crate) fn struct_field(&self, binding: BindingId, field: SymbolId) -> Option<AtomId> {
        match self.get(binding) {
            Some(Value::Struct { fields, .. }) => fields
                .iter()
                .find(|f| f.name.name == field)
                .map(|f| f.value),
            _ => None,
        }
    }
}

#[cfg(test)]
mod tests {
    use yuzu_core::adt::StringInterner;
    use yuzu_types::TypeCtx;

    use crate::AnfCtx;
    use crate::{Atom, Binding, BindingId, Const, Ident, StructFieldInit};

    use super::Environment;

    fn binding(anf: &mut AnfCtx, types: &TypeCtx, interner: &mut StringInterner) -> BindingId {
        anf.alloc_binding(Binding {
            name: Ident {
                name: interner.intern("b"),
            },
            ty: types.int64_ty(),
        })
    }

    #[test]
    fn unbound_binding_resolves_to_nothing() {
        let mut anf = AnfCtx::new();
        let types = TypeCtx::new();
        let mut interner = StringInterner::new();
        let b = binding(&mut anf, &types, &mut interner);

        let env = Environment::default();
        assert_eq!(env.atom(b), None);
        assert!(!env.is_aggregate_value(b));
        assert_eq!(env.struct_field(b, interner.intern("x")), None);
        assert!(env.list_elements(b).is_none());
    }

    #[test]
    fn rebinding_overwrites() {
        let mut anf = AnfCtx::new();
        let types = TypeCtx::new();
        let mut interner = StringInterner::new();
        let b = binding(&mut anf, &types, &mut interner);
        let one = anf.intern_atom(Atom::Const(Const::Int { value: 1.into() }));
        let two = anf.intern_atom(Atom::Const(Const::Int { value: 2.into() }));

        let mut env = Environment::default();
        env.bind_atom(b, one);
        assert_eq!(env.atom(b), Some(one));
        env.bind_atom(b, two);
        assert_eq!(env.atom(b), Some(two));
    }

    #[test]
    fn scalarized_struct_projects_by_name() {
        let mut anf = AnfCtx::new();
        let types = TypeCtx::new();
        let mut interner = StringInterner::new();
        let b = binding(&mut anf, &types, &mut interner);
        let value = anf.intern_atom(Atom::Const(Const::Int { value: 1.into() }));
        let field = interner.intern("v");

        let mut env = Environment::default();
        let origin = anf.alloc_expr(crate::Expr::Atom { value });
        env.bind_struct(
            b,
            interner.intern("P"),
            Box::new([StructFieldInit {
                name: Ident { name: field },
                value,
            }]),
            types.int64_ty(),
            origin,
        );
        assert!(env.is_aggregate_value(b));
        assert_eq!(env.atom(b), None);
        assert_eq!(env.struct_field(b, field), Some(value));
        assert_eq!(env.struct_field(b, interner.intern("missing")), None);
    }

    #[test]
    fn scalarized_list_exposes_elements() {
        let mut anf = AnfCtx::new();
        let types = TypeCtx::new();
        let mut interner = StringInterner::new();
        let b = binding(&mut anf, &types, &mut interner);
        let one = anf.intern_atom(Atom::Const(Const::Int { value: 1.into() }));
        let two = anf.intern_atom(Atom::Const(Const::Int { value: 2.into() }));

        let mut env = Environment::default();
        let origin = anf.alloc_expr(crate::Expr::Atom { value: one });
        env.bind_list(b, Box::new([one, two]), types.int64_ty(), origin);
        assert!(env.is_aggregate_value(b));
        assert_eq!(env.list_elements(b), Some([one, two].as_slice()));
    }
}
