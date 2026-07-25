"""Every join form is writable: each join type, both condition forms, and the
ways the joined relation can be named."""

from support import rows, sorted_rows

# employees joined to their department: dan (dept 9) matches nothing.
MATCHED = sorted_rows(("e1", "d1"), ("e2", "d2"), ("e3", "d1"))


def test_bare_join_is_inner():
    assert rows(
        "from employees e |> join departments d on e.dept_id == d.dept_id"
        " |> select e.id as ei, d.id as di"
    ) == MATCHED


def test_explicit_join_types():
    for kind in ["inner", "left", "right", "full"]:
        assert rows(
            f"from employees e |> {kind} join departments d on e.dept_id == d.dept_id"
            ' |> where e.name == "alice" |> select e.id as ei, d.id as di'
        ) == [("e1", "d1")]


def test_joined_relation_with_an_as_alias():
    assert rows(
        "from employees e |> join departments as d on e.dept_id == d.dept_id"
        " |> select e.id as ei, d.id as di"
    ) == MATCHED


def test_joined_relation_without_an_alias():
    """Without an alias the joined columns are only reachable bare, so this only
    works where the two sides share no column name."""
    assert rows(
        "from employees e |> join grades on e.dept_id == code |> select e.id as ei, label"
    ) == sorted_rows(("e1", "gold"), ("e2", "silver"), ("e3", "gold"))


def test_using_condition():
    assert rows(
        "from employees e |> join departments d using (dept_id) |> select e.id as ei, d.id as di"
    ) == MATCHED


def test_using_with_several_keys():
    assert rows(
        "from employees a |> join employees b using (id, dept_id) |> select id, dept_id"
    ) == sorted_rows(("e1", 1), ("e2", 2), ("e3", 1), ("e4", 9))


def test_chained_joins_of_mixed_types():
    assert rows(
        "from employees e |> join departments d on e.dept_id == d.dept_id"
        " |> left join projects p on p.dept_id == d.dept_id"
        " |> select e.id as ei, d.id as di, p.id as pi"
    ) == sorted_rows(("e1", "d1", "p1"), ("e2", "d2", "p2"), ("e3", "d1", "p1"))


def test_qualified_rename_after_a_join():
    assert rows(
        "from employees e |> join departments d on e.dept_id == d.dept_id"
        " |> rename e.id as ei, d.id as di |> select ei, di"
    ) == MATCHED


def test_self_join():
    assert rows(
        "from employees x |> join employees y on x.dept_id == y.dept_id"
        ' |> where x.name == "alice" |> select x.id as xi, y.id as yi'
    ) == sorted_rows(("e1", "e1"), ("e1", "e3"))
