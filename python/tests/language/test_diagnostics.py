"""What the language rejects, as the error reaches the Python API."""

from support import error_of


def test_ambiguous_bare_column():
    assert (
        error_of("from employees e |> join departments d on e.dept_id == d.dept_id |> select id")
        == "error: column `id` is ambiguous; qualify it with a relation alias"
    )


def test_unknown_column():
    assert error_of("from employees |> select nope") == "error: unresolved identifier `nope`"


def test_unknown_table():
    assert error_of("from nope |> select level") == "error: `nope` is not a relation"


def test_non_bool_predicate():
    assert (
        error_of("from employees |> where level")
        == "error: expected ``where` predicate` to be a `bool` type, but found `Int64`"
    )


def test_non_bool_join_condition():
    assert (
        error_of("from employees e |> join departments d on e.dept_id")
        == "error: expected ``on` condition` to be a `bool` type, but found `Int64`"
    )


def test_using_column_missing_from_a_side():
    assert (
        error_of("from employees e |> join departments d using (nope)")
        == "error: column nope not present in both relations"
    )
    assert (
        error_of("from employees e |> join departments d using (salary)")
        == "error: column salary not present in both relations"
    )


def test_operator_without_a_substrait_equivalent():
    assert (
        error_of("from employees |> select level ** 2 as p")
        == "error: operator `**` has no Substrait equivalent"
    )


def test_unbounded_recursion_is_rejected_at_compile_time():
    assert (
        error_of("fn f(n: int64) -> int64 { return f(n - 1) }\nfrom employees |> select f(3) as v")
        == "error: call to `f` could not be fully reduced"
    )
