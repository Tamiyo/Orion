"""Bindings and literals a query can be written against."""

from support import rows, sorted_rows


def test_let_binding():
    assert rows("let cap = 2\nfrom employees |> where level > cap |> select name") == [
        ("carol",)
    ]


def test_annotated_let_binding():
    assert rows(
        "let cap: int64 = 2\nfrom employees |> where level > cap |> select name"
    ) == [("carol",)]


def test_list_literal_and_membership():
    assert rows(
        "let ids = [1, 3]\nfrom employees |> where level in ids |> select name"
    ) == sorted_rows(("alice",), ("carol",))


def test_annotated_list_binding():
    assert rows(
        "let ids: List[int64] = [1, 3]\nfrom employees |> where level in ids |> select name"
    ) == sorted_rows(("alice",), ("carol",))


def test_literals_of_each_primitive_type():
    assert rows(
        'from employees |> where name == "alice"'
        ' |> select 1 as whole, 1.5 as fraction, true as yes, "s" as text'
    ) == [(1, 1.5, True, "s")]


def test_inline_list_membership():
    assert rows(
        'from employees |> where name in ["alice", "dan"] |> select level'
    ) == sorted_rows((1,), (2,))
