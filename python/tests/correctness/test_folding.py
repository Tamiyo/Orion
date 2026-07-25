"""Everything evaluates at compile time, so a folded value has to equal what
the expression would have computed."""

from support import rows, sorted_rows


def test_folded_call_matches_the_literal():
    folded = rows(
        "fn twice(n: int64) -> int64 { return n * 2 }\n"
        "from employees |> where level == twice(1) |> select name"
    )
    assert folded == rows("from employees |> where level == 2 |> select name")
    assert folded == sorted_rows(("bob",), ("dan",))


def test_folded_let_matches_the_literal():
    folded = rows("let limit = 1 + 1\nfrom employees |> where level > limit |> select name")
    assert folded == rows("from employees |> where level > 2 |> select name")
    assert folded == [("carol",)]


def test_assignment_folds_to_the_last_value():
    assert rows(
        "fn threshold() -> int64 { let mut x = 10\n x = 2\n return x }\n"
        "from employees |> where level > threshold() |> select name"
    ) == [("carol",)]


def test_folded_arithmetic_in_a_projection():
    assert rows(
        'from employees |> where name == "alice" |> select 2 * 3 + 1 as folded'
    ) == [(7,)]


def test_membership_over_a_constant_list_folds():
    assert rows(
        "let ids = [1, 3]\nfrom employees |> where level in ids |> select name"
    ) == sorted_rows(("alice",), ("carol",))
