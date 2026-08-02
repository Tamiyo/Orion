"""Bindings and literals a query can be written against."""

from support import rows, sorted_rows


def test_let_binding():
    query = """
        let cap = 2
        from employees
        |> where level > cap
        |> select name
    """
    assert rows(query) == [
        ("carol",)
    ]


def test_annotated_let_binding():
    query = """
        let cap: int64 = 2
        from employees
        |> where level > cap
        |> select name
    """
    assert rows(query) == [("carol",)]


def test_list_literal_and_membership():
    query = """
        let ids = [1, 3]
        from employees
        |> where level in ids
        |> select name
    """
    assert rows(query) == sorted_rows(("alice",), ("carol",))


def test_annotated_list_binding():
    query = """
        let ids: List[int64] = [1, 3]
        from employees
        |> where level in ids
        |> select name
    """
    assert rows(query) == sorted_rows(("alice",), ("carol",))


def test_literals_of_each_primitive_type():
    query = """
        from employees
        |> where name == "alice"
        |> select 1 as whole, 1.5 as fraction, true as yes, "s" as text
    """
    assert rows(query) == [(1, 1.5, True, "s")]


def test_inline_list_membership():
    query = """
        from employees
        |> where name in ["alice", "dan"]
        |> select level
    """
    assert rows(query) == sorted_rows((1,), (2,))
