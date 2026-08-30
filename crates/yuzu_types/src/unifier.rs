use crate::{InferKind, Type, TypeCtx, TypeId, TypeVariable};

pub struct TypeUnifier {
    filled: Vec<Option<TypeId>>,
}

impl TypeUnifier {
    pub fn new() -> Self {
        Self { filled: Vec::new() }
    }

    /// Mints a new type variable with no substitutions.
    pub fn fresh_var(&mut self, kind: InferKind, types: &mut TypeCtx) -> TypeId {
        let index = self.filled.len();
        self.filled.push(None);
        types.intern_ty(Type::TypeVar(TypeVariable { index, kind }))
    }

    /// Unifies two types `aid` and `bid`.
    ///
    /// When two types are the same, the types have been unified (base case).
    ///
    /// When either `aid` or `bid` is an error type, assume that the types match to avoid
    /// error propogation.
    ///
    /// When either `aid` or `bid` is a type variable, the substitutions for either type is
    /// filled/merged.
    ///
    /// When both types are list types, the inner types of the list are unified since lists are
    /// type constructors and not concrete types themselves.
    ///
    /// When both types are function types, the arguments and the types of the arguments must be
    /// unified to determine if both function types unify.
    pub fn unify(&mut self, aid: TypeId, bid: TypeId, types: &TypeCtx) -> bool {
        let aid = self.find(aid, types);
        let bid = self.find(bid, types);

        if aid == bid {
            return true;
        }

        if matches!(types.ty(aid), Type::Error) || matches!(types.ty(bid), Type::Error) {
            return true;
        }

        match (types.ty(aid), types.ty(bid)) {
            (Type::TypeVar(_), Type::TypeVar(_)) => self.merge(aid, bid, types),
            (Type::TypeVar(_), _) => self.fill(aid, bid, types),
            (_, Type::TypeVar(_)) => self.fill(bid, aid, types),

            (Type::List(a), Type::List(b)) => self.unify(a.inner, b.inner, types),

            (Type::Func(a), Type::Func(b)) => {
                if a.args.len() != b.args.len() {
                    return false;
                }

                for (a_arg_id, b_arg_id) in a.args.iter().zip(&b.args) {
                    if !self.unify(*a_arg_id, *b_arg_id, types) {
                        return false;
                    }
                }

                self.unify(a.ret_type, b.ret_type, types)
            }

            // Else
            _ => false,
        }
    }

    /// Resolves (collapses) a type `id` to its final type.
    ///
    /// If mapping is found, the found type is returned. If `id` resolves to a type variable, then
    /// the type inference the variable uses is considered. If the type variable uses delayed
    /// integer or float inference, then the type must eventually resolve to a variant of
    /// integer or float. Otherwise, the type points to a hole that should have been filled prior
    /// to unification.
    pub fn resolve(&mut self, id: TypeId, types: &mut TypeCtx) -> TypeId {
        let root = self.find(id, types);

        let kind = if let Type::TypeVar(var) = types.ty(root) {
            var.kind
        } else {
            return root;
        };

        match kind {
            InferKind::Int => types.int64_ty(),
            InferKind::Float => types.float64_ty(),
            InferKind::General => types.error_ty(),
        }
    }

    /// Finds the type that a type variable `id` maps to by following the substituion chain.
    ///
    /// Recursively walks type variable substitutions until a non-type variable is encountered,
    /// then returns that type.
    fn find(&mut self, id: TypeId, types: &TypeCtx) -> TypeId {
        let Type::TypeVar(var) = types.ty(id) else {
            return id;
        };

        let index = var.index;
        let Some(filling) = self.filled[index] else {
            return id;
        };

        let root = self.find(filling, types);

        self.filled[index] = Some(root);
        root
    }

    /// Fills the substitution of a type variable `id` with a concrete type.
    fn fill(&mut self, id: TypeId, concrete: TypeId, types: &TypeCtx) -> bool {
        let var = if let Type::TypeVar(var) = types.ty(id) {
            var
        } else {
            unreachable!("expected Type::TypeVariable but found: {:?}", id)
        };

        let ok = match var.kind {
            InferKind::General => true,
            InferKind::Int => types.ty(concrete).is_int(),
            InferKind::Float => types.ty(concrete).is_float(),
        };

        if !ok {
            return false;
        }

        self.filled[var.index] = Some(concrete);
        true
    }

    /// Merges the substitution of two type variables `aid` and `bid`.
    ///
    /// The type variables `aid` and `bid` have the same kind and must use general type inference,
    /// otherwise there is nothing to merge.
    /// merge.
    fn merge(&mut self, aid: TypeId, bid: TypeId, types: &TypeCtx) -> bool {
        let a = match types.ty(aid) {
            Type::TypeVar(var) => var,
            _ => unreachable!("INVARIANT - expected type variable but got :{:?}", aid),
        };

        let b = match types.ty(bid) {
            Type::TypeVar(var) => var,
            _ => unreachable!("INVARIANT - expected type variable but got :{:?}", bid),
        };

        if a.kind != b.kind && a.kind != InferKind::General && b.kind != InferKind::General {
            return false;
        }

        if b.kind == InferKind::General {
            self.filled[b.index] = Some(aid);
        } else {
            self.filled[a.index] = Some(bid);
        }

        true
    }
}

impl Default for TypeUnifier {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{FuncType, List, TypeParam};
    use string_interner::DefaultStringInterner;

    macro_rules! fixture {
        ($types:ident, $u:ident) => {
            let mut $types = TypeCtx::new();
            let mut $u = TypeUnifier::new();
        };
    }

    // --- concrete vs concrete ---

    #[test]
    fn identical_concrete_types_unify() {
        fixture!(types, u);
        let a = types.intern_ty(Type::Int64);
        let b = types.intern_ty(Type::Int64);
        assert!(u.unify(a, b, &types));
    }

    #[test]
    fn distinct_concrete_types_clash() {
        fixture!(types, u);
        let int = types.intern_ty(Type::Int64);
        let boolean = types.intern_ty(Type::Bool);
        assert!(!u.unify(int, boolean, &types));
    }

    // --- variable + concrete (fill) ---

    #[test]
    fn general_var_takes_any_concrete() {
        fixture!(types, u);
        let boolean = types.intern_ty(Type::Bool);
        let var = u.fresh_var(InferKind::General, &mut types);
        assert!(u.unify(var, boolean, &types));
        assert_eq!(u.resolve(var, &mut types), boolean);
    }

    #[test]
    fn fill_works_with_the_concrete_on_either_side() {
        fixture!(types, u);
        let int = types.intern_ty(Type::Int64);
        let var = u.fresh_var(InferKind::General, &mut types);
        assert!(u.unify(int, var, &types));
        assert_eq!(u.resolve(var, &mut types), int);
    }

    #[test]
    fn int_var_accepts_an_integer() {
        fixture!(types, u);
        let int = types.intern_ty(Type::Int32);
        let var = u.fresh_var(InferKind::Int, &mut types);
        assert!(u.unify(var, int, &types));
        assert_eq!(u.resolve(var, &mut types), int);
    }

    #[test]
    fn int_var_rejects_a_float() {
        fixture!(types, u);
        let float = types.intern_ty(Type::Float64);
        let var = u.fresh_var(InferKind::Int, &mut types);
        assert!(!u.unify(var, float, &types));
    }

    #[test]
    fn float_var_accepts_a_float() {
        fixture!(types, u);
        let float = types.intern_ty(Type::Float32);
        let var = u.fresh_var(InferKind::Float, &mut types);
        assert!(u.unify(var, float, &types));
        assert_eq!(u.resolve(var, &mut types), float);
    }

    #[test]
    fn float_var_rejects_an_integer() {
        fixture!(types, u);
        let int = types.intern_ty(Type::Int64);
        let var = u.fresh_var(InferKind::Float, &mut types);
        assert!(!u.unify(var, int, &types));
    }

    #[test]
    fn general_var_adopts_a_composite_type() {
        fixture!(types, u);
        let int = types.intern_ty(Type::Int64);
        let int_list = types.intern_ty(Type::List(List { inner: int }));
        let var = u.fresh_var(InferKind::General, &mut types);
        assert!(u.unify(var, int_list, &types));
        assert_eq!(u.resolve(var, &mut types), int_list);
    }

    // --- variable + variable (merge) ---

    #[test]
    fn merged_general_vars_share_a_filling() {
        fixture!(types, u);
        let int = types.intern_ty(Type::Int64);
        let a = u.fresh_var(InferKind::General, &mut types);
        let b = u.fresh_var(InferKind::General, &mut types);
        assert!(u.unify(a, b, &types));
        assert!(u.unify(b, int, &types));
        assert_eq!(u.resolve(a, &mut types), int);
        assert_eq!(u.resolve(b, &mut types), int);
    }

    #[test]
    fn merge_keeps_the_more_specific_kind() {
        fixture!(types, u);
        let float = types.intern_ty(Type::Float64);
        let int = types.intern_ty(Type::Int64);
        let general = u.fresh_var(InferKind::General, &mut types);
        let int_var = u.fresh_var(InferKind::Int, &mut types);
        assert!(u.unify(general, int_var, &types));
        assert!(!u.unify(general, float, &types));
        assert!(u.unify(general, int, &types));
        assert_eq!(u.resolve(general, &mut types), int);
    }

    #[test]
    fn int_and_float_vars_clash() {
        fixture!(types, u);
        let a = u.fresh_var(InferKind::Int, &mut types);
        let b = u.fresh_var(InferKind::Float, &mut types);
        assert!(!u.unify(a, b, &types));
    }

    // --- find / path compression ---

    #[test]
    fn chained_vars_resolve_through_the_root() {
        fixture!(types, u);
        let int = types.intern_ty(Type::Int64);
        let a = u.fresh_var(InferKind::General, &mut types);
        let b = u.fresh_var(InferKind::General, &mut types);
        let c = u.fresh_var(InferKind::General, &mut types);
        assert!(u.unify(a, b, &types));
        assert!(u.unify(b, c, &types));
        assert!(u.unify(c, int, &types));
        assert_eq!(u.resolve(a, &mut types), int);
    }

    #[test]
    fn unifying_a_var_with_itself_succeeds() {
        fixture!(types, u);
        let var = u.fresh_var(InferKind::General, &mut types);
        assert!(u.unify(var, var, &types));
    }

    // --- resolve ---

    #[test]
    fn unconstrained_vars_resolve_by_kind() {
        fixture!(types, u);
        let int64 = types.intern_ty(Type::Int64);
        let float64 = types.intern_ty(Type::Float64);
        let error = types.intern_ty(Type::Error);
        let general = u.fresh_var(InferKind::General, &mut types);
        let int = u.fresh_var(InferKind::Int, &mut types);
        let float = u.fresh_var(InferKind::Float, &mut types);
        assert_eq!(u.resolve(general, &mut types), error);
        assert_eq!(u.resolve(int, &mut types), int64);
        assert_eq!(u.resolve(float, &mut types), float64);
    }

    #[test]
    fn resolve_of_a_concrete_type_is_itself() {
        fixture!(types, u);
        let int = types.intern_ty(Type::Int64);
        assert_eq!(u.resolve(int, &mut types), int);
    }

    // --- lists ---

    #[test]
    fn lists_clash_on_their_element() {
        fixture!(types, u);
        let int = types.intern_ty(Type::Int64);
        let boolean = types.intern_ty(Type::Bool);
        let int_list = types.intern_ty(Type::List(List { inner: int }));
        let bool_list = types.intern_ty(Type::List(List { inner: boolean }));
        assert!(!u.unify(int_list, bool_list, &types));
    }

    #[test]
    fn unification_descends_into_lists_and_fills_nested_vars() {
        fixture!(types, u);
        let int = types.intern_ty(Type::Int64);
        let var = u.fresh_var(InferKind::General, &mut types);
        let var_list = types.intern_ty(Type::List(List { inner: var }));
        let int_list = types.intern_ty(Type::List(List { inner: int }));
        assert!(u.unify(var_list, int_list, &types));
        assert_eq!(u.resolve(var, &mut types), int);
    }

    #[test]
    fn a_list_does_not_unify_with_its_element() {
        fixture!(types, u);
        let int = types.intern_ty(Type::Int64);
        let int_list = types.intern_ty(Type::List(List { inner: int }));
        assert!(!u.unify(int_list, int, &types));
    }

    // --- functions ---

    #[test]
    fn identical_functions_unify() {
        fixture!(types, u);
        let int = types.intern_ty(Type::Int64);
        let boolean = types.intern_ty(Type::Bool);
        let f1 = types.intern_ty(Type::Func(FuncType {
            args: vec![int],
            ret_type: boolean,
        }));
        let f2 = types.intern_ty(Type::Func(FuncType {
            args: vec![int],
            ret_type: boolean,
        }));
        assert!(u.unify(f1, f2, &types));
    }

    #[test]
    fn functions_clash_on_an_argument() {
        fixture!(types, u);
        let int = types.intern_ty(Type::Int64);
        let boolean = types.intern_ty(Type::Bool);
        let f1 = types.intern_ty(Type::Func(FuncType {
            args: vec![int],
            ret_type: boolean,
        }));
        let f2 = types.intern_ty(Type::Func(FuncType {
            args: vec![boolean],
            ret_type: boolean,
        }));
        assert!(!u.unify(f1, f2, &types));
    }

    #[test]
    fn functions_clash_on_the_return_type() {
        fixture!(types, u);
        let int = types.intern_ty(Type::Int64);
        let boolean = types.intern_ty(Type::Bool);
        let string = types.intern_ty(Type::String);
        let f1 = types.intern_ty(Type::Func(FuncType {
            args: vec![int],
            ret_type: boolean,
        }));
        let f2 = types.intern_ty(Type::Func(FuncType {
            args: vec![int],
            ret_type: string,
        }));
        assert!(!u.unify(f1, f2, &types));
    }

    #[test]
    fn functions_clash_on_arity() {
        fixture!(types, u);
        let int = types.intern_ty(Type::Int64);
        let boolean = types.intern_ty(Type::Bool);
        let f1 = types.intern_ty(Type::Func(FuncType {
            args: vec![int],
            ret_type: boolean,
        }));
        let f2 = types.intern_ty(Type::Func(FuncType {
            args: vec![int, int],
            ret_type: boolean,
        }));
        assert!(!u.unify(f1, f2, &types));
    }

    #[test]
    fn unification_descends_into_functions_and_fills_vars() {
        fixture!(types, u);
        let int = types.intern_ty(Type::Int64);
        let boolean = types.intern_ty(Type::Bool);
        let arg_var = u.fresh_var(InferKind::General, &mut types);
        let ret_var = u.fresh_var(InferKind::General, &mut types);
        let f1 = types.intern_ty(Type::Func(FuncType {
            args: vec![arg_var],
            ret_type: ret_var,
        }));
        let f2 = types.intern_ty(Type::Func(FuncType {
            args: vec![int],
            ret_type: boolean,
        }));
        assert!(u.unify(f1, f2, &types));
        assert_eq!(u.resolve(arg_var, &mut types), int);
        assert_eq!(u.resolve(ret_var, &mut types), boolean);
    }

    // --- type params ---

    #[test]
    fn identical_type_params_unify() {
        fixture!(types, u);
        let mut strings = DefaultStringInterner::default();
        let name = strings.get_or_intern("T");
        let a = types.intern_ty(Type::TypeParam(TypeParam { name, index: 0 }));
        let b = types.intern_ty(Type::TypeParam(TypeParam { name, index: 0 }));
        assert!(u.unify(a, b, &types));
    }

    #[test]
    fn distinct_type_params_clash() {
        fixture!(types, u);
        let mut strings = DefaultStringInterner::default();
        let t = strings.get_or_intern("T");
        let v = strings.get_or_intern("U");
        let a = types.intern_ty(Type::TypeParam(TypeParam { name: t, index: 0 }));
        let b = types.intern_ty(Type::TypeParam(TypeParam { name: v, index: 1 }));
        assert!(!u.unify(a, b, &types));
    }
}
