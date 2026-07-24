use std::cmp::Ordering;

use yuzu_core::adt::{Float, Int};

use crate::anf::{Const, Op};

pub(crate) fn fold(op: Op, args: &[Const]) -> Option<Const> {
    match op {
        Op::Add => arithmetic(args, Int::checked_add, Float::checked_add),
        Op::Sub => arithmetic(args, Int::checked_sub, Float::checked_sub),
        Op::Mul => arithmetic(args, Int::checked_mul, Float::checked_mul),
        Op::Div => arithmetic(args, Int::checked_div, Float::checked_div),
        Op::Pow => arithmetic(args, Int::checked_pow, Float::checked_pow),
        Op::ShiftLeft => integer(args, Int::checked_shl),
        Op::ShiftRight => integer(args, Int::checked_shr),
        Op::And => logical(args, |a, b| a && b),
        Op::Or => logical(args, |a, b| a || b),
        Op::Eq => equality(args, false),
        Op::Neq => equality(args, true),
        Op::Lt => ordering(args, |o| o == Ordering::Less),
        Op::Lte => ordering(args, |o| o != Ordering::Greater),
        Op::Gt => ordering(args, |o| o == Ordering::Greater),
        Op::Gte => ordering(args, |o| o != Ordering::Less),
        Op::UnaryPos => match args {
            [constant @ (Const::Int { .. } | Const::Float { .. })] => Some(*constant),
            _ => None,
        },
        Op::UnaryNeg => match args {
            [Const::Int { value }] => value.checked_neg().map(|value| Const::Int { value }),
            [Const::Float { value }] => Some(Const::Float {
                value: value.checked_neg(),
            }),
            _ => None,
        },
        Op::UnaryNot => match args {
            [Const::Bool { value }] => Some(Const::Bool { value: !value }),
            _ => None,
        },
        // Membership tests the needle against a list, not a constant; the
        // reducer folds them through the environment (see `fold_membership`).
        Op::In | Op::NotIn => None,
    }
}

fn arithmetic(
    args: &[Const],
    on_int: fn(Int, Int) -> Option<Int>,
    on_float: fn(Float, Float) -> Option<Float>,
) -> Option<Const> {
    match args {
        [Const::Int { value: a }, Const::Int { value: b }] => {
            on_int(*a, *b).map(|value| Const::Int { value })
        }
        [Const::Float { value: a }, Const::Float { value: b }] => {
            on_float(*a, *b).map(|value| Const::Float { value })
        }
        _ => None,
    }
}

fn integer(args: &[Const], on_int: fn(Int, Int) -> Option<Int>) -> Option<Const> {
    match args {
        [Const::Int { value: a }, Const::Int { value: b }] => {
            on_int(*a, *b).map(|value| Const::Int { value })
        }
        _ => None,
    }
}

fn logical(args: &[Const], op: fn(bool, bool) -> bool) -> Option<Const> {
    match args {
        [Const::Bool { value: a }, Const::Bool { value: b }] => {
            Some(Const::Bool { value: op(*a, *b) })
        }
        _ => None,
    }
}

fn equality(args: &[Const], negate: bool) -> Option<Const> {
    let equal = match args {
        [Const::Int { value: a }, Const::Int { value: b }] => a.compare(*b)? == Ordering::Equal,
        [Const::Float { value: a }, Const::Float { value: b }] => a.compare(*b)? == Ordering::Equal,
        [Const::Bool { value: a }, Const::Bool { value: b }] => a == b,
        // Interned strings are equal exactly when their symbols are.
        [Const::String { value: a }, Const::String { value: b }] => a == b,
        _ => return None,
    };
    Some(Const::Bool {
        value: equal ^ negate,
    })
}

fn ordering(args: &[Const], predicate: fn(Ordering) -> bool) -> Option<Const> {
    let order = match args {
        [Const::Int { value: a }, Const::Int { value: b }] => a.compare(*b)?,
        [Const::Float { value: a }, Const::Float { value: b }] => a.compare(*b)?,
        _ => return None,
    };
    Some(Const::Bool {
        value: predicate(order),
    })
}

#[cfg(test)]
mod tests {
    use expect_test::expect;
    use yuzu_core::adt::StringInterner;

    use crate::anf::{Const, Op};
    use crate::reduction::test_support::{TABLE, check};

    use super::fold;

    fn int(value: i64) -> Const {
        Const::Int {
            value: value.into(),
        }
    }

    #[test]
    fn overflow_is_left_unfolded() {
        assert!(fold(Op::Add, &[int(i64::MAX), int(1)]).is_none());
        assert!(fold(Op::Mul, &[int(i64::MAX), int(2)]).is_none());
    }

    #[test]
    fn division_by_zero_is_left_unfolded() {
        assert!(fold(Op::Div, &[int(1), int(0)]).is_none());
    }

    #[test]
    fn negating_the_minimum_is_left_unfolded() {
        assert!(fold(Op::UnaryNeg, &[int(i64::MIN)]).is_none());
    }

    #[test]
    fn shifting_past_the_width_is_left_unfolded() {
        assert!(fold(Op::ShiftLeft, &[int(1), int(64)]).is_none());
        assert!(fold(Op::ShiftRight, &[int(1), int(-1)]).is_none());
    }

    #[test]
    fn mixed_operand_types_are_left_unfolded() {
        let float = Const::Float { value: 1.0.into() };
        assert!(fold(Op::Add, &[int(1), float]).is_none());
        assert!(fold(Op::Eq, &[int(1), float]).is_none());
    }

    #[test]
    fn string_inequality_folds() {
        let mut interner = StringInterner::new();
        let a = Const::String {
            value: interner.intern("a"),
        };
        let b = Const::String {
            value: interner.intern("b"),
        };
        assert!(fold(Op::Neq, &[a, b]) == Some(Const::Bool { value: true }));
        assert!(fold(Op::Eq, &[a, a]) == Some(Const::Bool { value: true }));
    }

    #[test]
    fn membership_is_never_folded_here() {
        assert!(fold(Op::In, &[int(1), int(1)]).is_none());
        assert!(fold(Op::NotIn, &[int(1), int(1)]).is_none());
    }

    #[test]
    fn folds_arithmetic_and_propagates() {
        check(
            &format!("{TABLE}let x = 1 + 2 * 3\nlet y = x + 4\nfrom t |> select x, y"),
            expect![[r#"
                struct Row { a, b }
                table t
                from t
                  |> select 7i64, 11i64
            "#]],
        );
    }

    #[test]
    fn folds_comparison_and_logical() {
        check(
            &format!("{TABLE}let x = (2 > 1) and (3 == 3)\nlet y = not x\nfrom t |> select x, y"),
            expect![[r#"
                struct Row { a, b }
                table t
                from t
                  |> select true, false
            "#]],
        );
    }

    #[test]
    fn folds_unary_and_shifts() {
        check(
            &format!("{TABLE}from t |> select -5 as a, 1 << 4 as b, 255 >> 2 as c"),
            expect![[r#"
                struct Row { a, b }
                table t
                from t
                  |> select -5i64 as a, 16i64 as b, 63i64 as c
            "#]],
        );
    }

    #[test]
    fn folds_float_arithmetic_and_negation() {
        check(
            &format!("{TABLE}let x = 1.5 + 2.5\nlet y = x * 2.0\nfrom t |> select x, y, -y as ny"),
            expect![[r#"
                struct Row { a, b }
                table t
                from t
                  |> select 4f64, 8f64, -8f64 as ny
            "#]],
        );
    }

    #[test]
    fn folds_string_equality() {
        check(
            &format!(
                "{TABLE}let n = \"jon\"\nfrom t |> select n == \"jon\" as same, n == \"ann\" as other"
            ),
            expect![[r#"
                struct Row { a, b }
                table t
                from t
                  |> select true as same, false as other
            "#]],
        );
    }
}
