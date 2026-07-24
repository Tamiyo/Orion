use yuzu_types::TypeId;

use crate::{Op, infer::InferCtx};

impl Op {
    pub(crate) fn resolve(self, args: &[TypeId], infer: &mut InferCtx) -> TypeId {
        match self {
            Op::Add | Op::Sub | Op::Mul | Op::Div | Op::Pow => resolve_arithmetic(infer, args),
            Op::ShiftLeft | Op::ShiftRight => resolve_shift(infer, args),
            Op::And | Op::Or => resolve_logical(infer, args),
            Op::Eq | Op::Neq | Op::Lt | Op::Lte | Op::Gt | Op::Gte => {
                resolve_comparison(infer, args)
            }
            Op::In | Op::NotIn => resolve_membership(infer, args),
            Op::UnaryPos | Op::UnaryNeg => resolve_unary_arithmetic(infer, args),
            Op::UnaryNot => resolve_unary_logical(infer, args),
        }
    }
}

fn binary_operands(args: &[TypeId]) -> (TypeId, TypeId) {
    assert_eq!(
        args.len(),
        2,
        "binary operator expects 2 operands, found {}",
        args.len()
    );
    (args[0], args[1])
}

fn unary_operand(args: &[TypeId]) -> TypeId {
    assert_eq!(
        args.len(),
        1,
        "unary operator expects 1 operand, found {}",
        args.len()
    );
    args[0]
}

fn resolve_arithmetic(infer: &mut InferCtx, args: &[TypeId]) -> TypeId {
    let (lhs, rhs) = binary_operands(args);
    if infer.unify(lhs, rhs) && infer.is_numeric(lhs) {
        lhs
    } else {
        infer.types.error_ty()
    }
}

fn resolve_shift(infer: &mut InferCtx, args: &[TypeId]) -> TypeId {
    let (value, count) = binary_operands(args);
    if infer.is_int(value) && infer.is_int(count) {
        value
    } else {
        infer.types.error_ty()
    }
}

fn resolve_logical(infer: &mut InferCtx, args: &[TypeId]) -> TypeId {
    let (lhs, rhs) = binary_operands(args);
    let bool_ty = infer.types.bool_ty();
    if infer.unify(lhs, bool_ty) && infer.unify(rhs, bool_ty) {
        bool_ty
    } else {
        infer.types.error_ty()
    }
}

fn resolve_comparison(infer: &mut InferCtx, args: &[TypeId]) -> TypeId {
    let (lhs, rhs) = binary_operands(args);
    if infer.unify(lhs, rhs) {
        infer.types.bool_ty()
    } else {
        infer.types.error_ty()
    }
}

fn resolve_membership(infer: &mut InferCtx, args: &[TypeId]) -> TypeId {
    let (lhs, rhs) = binary_operands(args);

    // List membership: `x in xs` where `xs: List[x]`.
    let resolved = infer.resolve(rhs);
    if let yuzu_types::Type::List(list) = infer.types.ty(resolved) {
        let inner = list.inner;
        return if infer.unify(lhs, inner) {
            infer.types.bool_ty()
        } else {
            infer.types.error_ty()
        };
    }

    // String containment: `"a" in "abc"`.
    let str_ty = infer.types.str_ty();
    if infer.unify(lhs, str_ty) && infer.unify(rhs, str_ty) {
        infer.types.bool_ty()
    } else {
        infer.types.error_ty()
    }
}

fn resolve_unary_arithmetic(infer: &mut InferCtx, args: &[TypeId]) -> TypeId {
    let operand = unary_operand(args);
    if infer.is_numeric(operand) {
        operand
    } else {
        infer.types.error_ty()
    }
}

fn resolve_unary_logical(infer: &mut InferCtx, args: &[TypeId]) -> TypeId {
    let operand = unary_operand(args);
    let bool_ty = infer.types.bool_ty();
    if infer.unify(operand, bool_ty) {
        bool_ty
    } else {
        infer.types.error_ty()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use yuzu_types::{InferKind, TypeCtx};

    macro_rules! fixture {
        ($types:ident, $infer:ident) => {
            let mut $types = TypeCtx::new();
            let mut $infer = InferCtx::new(&mut $types);
        };
    }

    fn check(infer: &mut InferCtx, op: Op, args: &[TypeId], expected: TypeId) {
        let got = op.resolve(args, infer);
        let got = infer.resolve(got);
        let expected = infer.resolve(expected);
        assert_eq!(got, expected, "op {:?}", op);
    }

    #[test]
    fn arithmetic_yields_operand_type() {
        fixture!(types, infer);
        let int = infer.types.int64_ty();
        for op in [Op::Add, Op::Sub, Op::Mul, Op::Div, Op::Pow] {
            check(&mut infer, op, &[int, int], int);
        }
    }

    #[test]
    fn arithmetic_keeps_open_int_hole() {
        fixture!(types, infer);
        let a = infer.fresh_var(InferKind::Int);
        let b = infer.fresh_var(InferKind::Int);
        let int = infer.types.int64_ty();
        check(&mut infer, Op::Add, &[a, b], int);
    }

    #[test]
    fn arithmetic_rejects_non_numeric() {
        fixture!(types, infer);
        let boolean = infer.types.bool_ty();
        let error = infer.types.error_ty();
        check(&mut infer, Op::Add, &[boolean, boolean], error);
    }

    #[test]
    fn arithmetic_rejects_mixed_numerics() {
        fixture!(types, infer);
        let int = infer.types.int64_ty();
        let float = infer.types.float64_ty();
        let error = infer.types.error_ty();
        check(&mut infer, Op::Add, &[int, float], error);
    }

    #[test]
    fn shift_yields_left_operand() {
        fixture!(types, infer);
        let int = infer.types.int64_ty();
        for op in [Op::ShiftLeft, Op::ShiftRight] {
            check(&mut infer, op, &[int, int], int);
        }
    }

    #[test]
    fn shift_rejects_non_int() {
        fixture!(types, infer);
        let int = infer.types.int64_ty();
        let boolean = infer.types.bool_ty();
        let error = infer.types.error_ty();
        check(&mut infer, Op::ShiftLeft, &[int, boolean], error);
    }

    #[test]
    fn logical_yields_bool() {
        fixture!(types, infer);
        let boolean = infer.types.bool_ty();
        for op in [Op::And, Op::Or] {
            check(&mut infer, op, &[boolean, boolean], boolean);
        }
    }

    #[test]
    fn logical_rejects_non_bool() {
        fixture!(types, infer);
        let boolean = infer.types.bool_ty();
        let int = infer.types.int64_ty();
        let error = infer.types.error_ty();
        check(&mut infer, Op::And, &[boolean, int], error);
    }

    #[test]
    fn comparison_yields_bool() {
        fixture!(types, infer);
        let int = infer.types.int64_ty();
        let boolean = infer.types.bool_ty();
        for op in [Op::Eq, Op::Neq, Op::Lt, Op::Lte, Op::Gt, Op::Gte] {
            check(&mut infer, op, &[int, int], boolean);
        }
    }

    #[test]
    fn comparison_rejects_mismatched_operands() {
        fixture!(types, infer);
        let int = infer.types.int64_ty();
        let boolean = infer.types.bool_ty();
        let error = infer.types.error_ty();
        check(&mut infer, Op::Eq, &[int, boolean], error);
    }

    #[test]
    fn membership_yields_bool() {
        fixture!(types, infer);
        let string = infer.types.str_ty();
        let boolean = infer.types.bool_ty();
        for op in [Op::In, Op::NotIn] {
            check(&mut infer, op, &[string, string], boolean);
        }
    }

    #[test]
    fn membership_accepts_list_of_element() {
        fixture!(types, infer);
        let int = infer.types.int64_ty();
        let list = infer.types.list_ty(int);
        let boolean = infer.types.bool_ty();
        for op in [Op::In, Op::NotIn] {
            check(&mut infer, op, &[int, list], boolean);
        }
    }

    #[test]
    fn membership_rejects_mismatched_element() {
        fixture!(types, infer);
        let int = infer.types.int64_ty();
        let float = infer.types.float64_ty();
        let list = infer.types.list_ty(int);
        let error = infer.types.error_ty();
        check(&mut infer, Op::In, &[float, list], error);
    }

    #[test]
    fn membership_rejects_non_str() {
        fixture!(types, infer);
        let int = infer.types.int64_ty();
        let error = infer.types.error_ty();
        check(&mut infer, Op::In, &[int, int], error);
    }

    #[test]
    fn unary_arithmetic_yields_operand_type() {
        fixture!(types, infer);
        let int = infer.types.int64_ty();
        for op in [Op::UnaryPos, Op::UnaryNeg] {
            check(&mut infer, op, &[int], int);
        }
    }

    #[test]
    fn unary_arithmetic_rejects_non_numeric() {
        fixture!(types, infer);
        let boolean = infer.types.bool_ty();
        let error = infer.types.error_ty();
        check(&mut infer, Op::UnaryNeg, &[boolean], error);
    }

    #[test]
    fn unary_logical_yields_bool() {
        fixture!(types, infer);
        let boolean = infer.types.bool_ty();
        check(&mut infer, Op::UnaryNot, &[boolean], boolean);
    }

    #[test]
    fn unary_logical_rejects_non_bool() {
        fixture!(types, infer);
        let int = infer.types.int64_ty();
        let error = infer.types.error_ty();
        check(&mut infer, Op::UnaryNot, &[int], error);
    }

    #[test]
    fn arithmetic_yields_float_operand_type() {
        fixture!(types, infer);
        let float = infer.types.float64_ty();
        for op in [Op::Add, Op::Sub, Op::Mul, Op::Div, Op::Pow] {
            check(&mut infer, op, &[float, float], float);
        }
    }

    #[test]
    fn unary_arithmetic_pos_rejects_non_numeric() {
        fixture!(types, infer);
        let boolean = infer.types.bool_ty();
        let error = infer.types.error_ty();
        check(&mut infer, Op::UnaryPos, &[boolean], error);
    }

    #[test]
    #[should_panic(expected = "binary operator expects 2 operands")]
    fn binary_op_rejects_wrong_arity() {
        fixture!(types, infer);
        let int = infer.types.int64_ty();
        Op::Add.resolve(&[int], &mut infer);
    }

    #[test]
    #[should_panic(expected = "unary operator expects 1 operand")]
    fn unary_op_rejects_wrong_arity() {
        fixture!(types, infer);
        let boolean = infer.types.bool_ty();
        Op::UnaryNot.resolve(&[boolean, boolean], &mut infer);
    }
}
