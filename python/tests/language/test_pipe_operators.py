"""Every `|>` stage is writable, alone and chained."""

from support import rows, sorted_rows


def test_select():
    query = """
        from employees
        |> select name
    """
    assert rows(query) == sorted_rows(
        ("alice",), ("bob",), ("carol",), ("dan",)
    )


def test_select_with_an_alias():
    query = """
        from employees
        |> where name == "alice"
        |> select level as lvl
    """
    assert rows(query) == [(1,)]


def test_where():
    query = """
        from employees
        |> where level == 3
        |> select name
    """
    assert rows(query) == [("carol",)]


def test_extend():
    query = """
        from employees
        |> where name == "alice"
        |> extend level + 1 as next
        |> select name, next
    """
    assert rows(query) == [("alice", 2)]


def test_drop():
    query = """
        from employees
        |> where name == "alice"
        |> drop id, dept_id, salary, active, rating
    """
    assert rows(query) == [("alice", 1)]


def test_rename():
    query = """
        from employees
        |> where name == "alice"
        |> rename level as lvl
        |> select name, lvl
    """
    assert rows(query) == [("alice", 1)]


def test_distinct():
    query = """
        from employees
        |> select level
        |> distinct
    """
    assert rows(query) == sorted_rows((1,), (2,), (3,))


def test_every_stage_chained():
    query = """
        from employees e
        |> where e.level > 1
        |> extend e.level * 10 as big
        |> rename e.name as who
        |> drop id, dept_id, salary, active, rating
        |> select who, big
        |> distinct
    """
    assert rows(query) == sorted_rows(("bob", 20), ("carol", 30), ("dan", 20))


def test_set():
    query = """
        from employees
        |> set level = level * 10
        |> where name == "alice"
        |> select level
    """
    assert rows(query) == [
        (10,)
    ]


def test_limit():
    query = """
        from employees
        |> limit 2
        |> select name
    """
    assert len(rows(query)) == 2


def test_limit_with_offset():
    query = """
        from employees
        |> limit 2 offset 1
        |> select name
    """
    assert len(rows(query)) == 2


def test_alias():
    query = """
        from employees
        |> as staff
        |> where staff.name == "alice"
        |> select staff.level
    """
    assert rows(query) == [
        (1,)
    ]
