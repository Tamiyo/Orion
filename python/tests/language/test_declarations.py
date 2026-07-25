"""Declaring row types and relations, and naming them in a query."""

from support import rows, sorted_rows

INLINE = """
table people = { name: str, level: int64 }
"""


def test_struct_backed_table():
    assert rows("from employees |> select name") == sorted_rows(
        ("alice",), ("bob",), ("carol",), ("dan",)
    )


def test_inline_table():
    assert rows(
        "from people |> select name",
        schema=INLINE,
        tables={"people": {"name": ["ada", "grace"], "level": [1, 2]}},
    ) == sorted_rows(("ada",), ("grace",))


def test_several_relations_in_one_program():
    assert rows("from departments |> select name") == sorted_rows(
        ("eng",), ("sales",), ("ops",)
    )
    assert rows("from grades |> select label") == sorted_rows(
        ("gold",), ("silver",), ("bronze",)
    )


def test_from_without_an_alias():
    assert rows('from employees |> where name == "alice" |> select level') == [(1,)]


def test_from_with_a_bare_alias():
    assert rows('from employees e |> where e.name == "alice" |> select e.level') == [(1,)]


def test_from_with_an_as_alias():
    assert rows('from employees as e |> where e.name == "alice" |> select e.level') == [(1,)]


def test_every_declared_type_is_readable():
    assert rows(
        'from employees |> where name == "alice"'
        " |> select id, name, dept_id, level, salary, active, rating"
    ) == [("e1", "alice", 1, 1, 120000, True, 1.5)]
