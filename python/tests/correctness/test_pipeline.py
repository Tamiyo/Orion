"""Each `|>` stage transforms the rows the way it should."""

from support import rows, sorted_rows


def test_where_keeps_only_matching_rows():
    assert rows("from employees |> where salary > 100000 |> select name") == sorted_rows(
        ("alice",), ("carol",)
    )


def test_where_matching_nothing_is_empty():
    assert rows("from employees |> where salary > 500000 |> select name") == []


def test_select_projects_in_the_written_order():
    assert rows(
        'from employees |> where name == "alice" |> select level, name, active'
    ) == [(1, "alice", True)]


def test_extend_appends_and_keeps_the_input_columns():
    assert rows(
        'from employees |> where name == "carol" |> extend salary / 12 as monthly'
        " |> select name, salary, monthly"
    ) == [("carol", 240000, 20000)]


def test_extend_is_visible_to_later_stages():
    assert rows(
        "from employees |> extend level * 10 as big |> where big > 20 |> select name, big"
    ) == [("carol", 30)]


def test_drop_removes_only_the_named_columns():
    assert rows(
        'from employees |> where name == "alice" |> drop id, dept_id, salary, active, rating'
    ) == [("alice", 1)]


def test_rename_changes_the_name_and_keeps_the_value():
    assert rows(
        'from employees |> where name == "alice" |> rename level as lvl |> select name, lvl'
    ) == [("alice", 1)]


def test_distinct_drops_repeated_rows():
    assert rows("from employees |> select level |> distinct") == sorted_rows((1,), (2,), (3,))


def test_distinct_considers_the_whole_row():
    assert rows("from employees |> select level, active |> distinct") == sorted_rows(
        (1, True), (2, False), (3, True)
    )


def test_stages_compose():
    assert rows(
        "from employees e"
        " |> where e.active"
        " |> extend e.salary / 12 as monthly"
        " |> rename e.name as who"
        " |> drop id, dept_id, salary, active, rating"
        " |> select who, monthly"
    ) == sorted_rows(("alice", 10000), ("carol", 20000))
