"""Declaring row types and relations, and naming them in a query."""

from support import rows, sorted_rows


def test_struct_backed_table():
    query = """
        from employees
        |> select name
    """
    assert rows(query) == sorted_rows(
        ("alice",), ("bob",), ("carol",), ("dan",)
    )


def test_inline_table():
    query = """
        from people
        |> select name
    """
    assert rows(
        query,
        tables={"people": {"name": ["ada", "grace"], "level": [1, 2]}},
    ) == sorted_rows(("ada",), ("grace",))


def test_several_relations_in_one_program():
    assert rows("""
        from departments
        |> select name
    """) == sorted_rows(
        ("eng",), ("sales",), ("ops",)
    )
    assert rows("""
        from grades
        |> select label
    """) == sorted_rows(
        ("gold",), ("silver",), ("bronze",)
    )


def test_from_without_an_alias():
    query = """
        from employees
        |> where name == "alice"
        |> select level
    """
    assert rows(query) == [(1,)]


def test_from_with_a_bare_alias():
    query = """
        from employees e
        |> where e.name == "alice"
        |> select e.level
    """
    assert rows(query) == [(1,)]


def test_from_with_an_as_alias():
    query = """
        from employees as e
        |> where e.name == "alice"
        |> select e.level
    """
    assert rows(query) == [(1,)]


def test_every_declared_type_is_readable():
    query = """
        from employees
        |> where name == "alice"
        |> select id, name, dept_id, level, salary, active, rating
    """
    assert rows(query) == [("e1", "alice", 1, 1, 120000, True, 1.5)]
