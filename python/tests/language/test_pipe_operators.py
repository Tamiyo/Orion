"""Every `|>` stage is writable, alone and chained."""

from support import rows, sorted_rows


def test_select():
    assert rows("from employees |> select name") == sorted_rows(
        ("alice",), ("bob",), ("carol",), ("dan",)
    )


def test_select_with_an_alias():
    assert rows('from employees |> where name == "alice" |> select level as lvl') == [(1,)]


def test_where():
    assert rows("from employees |> where level == 3 |> select name") == [("carol",)]


def test_extend():
    assert rows(
        'from employees |> where name == "alice" |> extend level + 1 as next |> select name, next'
    ) == [("alice", 2)]


def test_drop():
    assert rows(
        'from employees |> where name == "alice" |> drop id, dept_id, salary, active, rating'
    ) == [("alice", 1)]


def test_rename():
    assert rows(
        'from employees |> where name == "alice" |> rename level as lvl |> select name, lvl'
    ) == [("alice", 1)]


def test_distinct():
    assert rows("from employees |> select level |> distinct") == sorted_rows((1,), (2,), (3,))


def test_every_stage_chained():
    assert rows(
        "from employees e"
        " |> where e.level > 1"
        " |> extend e.level * 10 as big"
        " |> rename e.name as who"
        " |> drop id, dept_id, salary, active, rating"
        " |> select who, big"
        " |> distinct"
    ) == sorted_rows(("bob", 20), ("carol", 30), ("dan", 20))
