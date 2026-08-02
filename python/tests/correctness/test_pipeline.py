"""Each `|>` stage transforms the rows the way it should."""

from support import rows, sorted_rows


def test_where_keeps_only_matching_rows():
    query = """
        from employees
        |> where salary > 100000
        |> select name
    """
    assert rows(query) == sorted_rows(
        ("alice",), ("carol",)
    )


def test_where_matching_nothing_is_empty():
    query = """
        from employees
        |> where salary > 500000
        |> select name
    """
    assert rows(query) == []


def test_select_projects_in_the_written_order():
    query = """
        from employees
        |> where name == "alice"
        |> select level, name, active
    """
    assert rows(query) == [(1, "alice", True)]


def test_extend_appends_and_keeps_the_input_columns():
    query = """
        from employees
        |> where name == "carol"
        |> extend salary / 12 as monthly
        |> select name, salary, monthly
    """
    assert rows(query) == [("carol", 240000, 20000)]


def test_extend_is_visible_to_later_stages():
    query = """
        from employees
        |> extend level * 10 as big
        |> where big > 20
        |> select name, big
    """
    assert rows(query) == [("carol", 30)]


def test_drop_removes_only_the_named_columns():
    query = """
        from employees
        |> where name == "alice"
        |> drop id, dept_id, salary, active, rating
    """
    assert rows(query) == [("alice", 1)]


def test_rename_changes_the_name_and_keeps_the_value():
    query = """
        from employees
        |> where name == "alice"
        |> rename level as lvl
        |> select name, lvl
    """
    assert rows(query) == [("alice", 1)]


def test_distinct_drops_repeated_rows():
    query = """
        from employees
        |> select level
        |> distinct
    """
    assert rows(query) == sorted_rows((1,), (2,), (3,))


def test_distinct_considers_the_whole_row():
    query = """
        from employees
        |> select level, active
        |> distinct
    """
    assert rows(query) == sorted_rows(
        (1, True), (2, False), (3, True)
    )


def test_stages_compose():
    query = """
        from employees e
        |> where e.active
        |> extend e.salary / 12 as monthly
        |> rename e.name as who
        |> drop id, dept_id, salary, active, rating
        |> select who, monthly
    """
    assert rows(query) == sorted_rows(("alice", 10000), ("carol", 20000))


def test_set_replaces_a_column_in_place():
    """The row keeps its shape and order; only the named column's value changes."""
    query = """
        from employees
        |> set level = 99
        |> where name == "alice"
        |> select id, name, dept_id, level
    """
    assert rows(query) == [("e1", "alice", 1, 99)]


def test_set_sees_the_input_row():
    query = """
        from employees
        |> set level = level + salary
        |> where name == "alice"
        |> select level
    """
    assert rows(query) == [(120001,)]


def test_set_several_columns_at_once():
    query = """
        from employees
        |> set level = 1, salary = 2
        |> where name == "bob"
        |> select level, salary
    """
    assert rows(query) == [(1, 2)]


def test_limit_caps_the_row_count():
    assert len(rows("""
        from employees
        |> limit 3
        |> select name
    """)) == 3
    assert len(rows("""
        from employees
        |> limit 99
        |> select name
    """)) == 4
    assert rows("""
        from employees
        |> limit 0
        |> select name
    """) == []


def test_offset_skips_before_limiting():
    all_names = {row[0] for row in rows("""
        from employees
        |> select name
    """)}
    skipped = {row[0] for row in rows("""
        from employees
        |> limit 4 offset 2
        |> select name
    """)}
    assert len(skipped) == 2
    assert skipped < all_names


def test_alias_renames_the_whole_row():
    query = """
        from employees e
        |> as staff
        |> where staff.name == "alice"
        |> select staff.id
    """
    assert rows(query) == [("e1",)]


def test_alias_disambiguates_after_a_join():
    """`as` is what lets a join's own columns be qualified again."""
    query = """
        from employees e
        |> join departments d on e.dept_id == d.dept_id
        |> select e.id as eid, d.name as dept
        |> as joined
        |> select joined.eid, joined.dept
    """
    assert rows(query) == sorted_rows(("e1", "eng"), ("e2", "sales"), ("e3", "eng"))
