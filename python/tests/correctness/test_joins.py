"""Joins match the right rows, keep the right columns, and resolve each
reference to the side it names."""

from support import rows, sorted_rows

# alice and carol are in dept 1, bob in dept 2, dan in dept 9 (no department).
# Department `ops` (dept 3) has no employee.
MATCHED = sorted_rows(("alice", "eng"), ("bob", "sales"), ("carol", "eng"))
# The same employees joined to `grades`, whose codes mirror the department ids.
GRADED = sorted_rows(("alice", "gold"), ("bob", "silver"), ("carol", "gold"))


def test_inner_join_keeps_only_matching_rows():
    assert (
        rows(
            "from employees e |> join departments d on e.dept_id == d.dept_id"
            " |> select e.name as who, d.name as dept"
        )
        == MATCHED
    )


def test_inner_join_without_matches_is_empty():
    assert rows(
        "from employees e |> join departments d on e.dept_id == d.dept_id"
        " |> where e.salary > 500000 |> select e.name as who"
    ) == []


def test_left_join_keeps_unmatched_left_rows():
    assert rows(
        "from employees e |> left join departments d on e.dept_id == d.dept_id"
        " |> select e.name as who, d.name as dept"
    ) == sorted_rows(*MATCHED, ("dan", None))


def test_right_join_keeps_unmatched_right_rows():
    assert rows(
        "from employees e |> right join departments d on e.dept_id == d.dept_id"
        " |> select e.name as who, d.name as dept"
    ) == sorted_rows(*MATCHED, (None, "ops"))


def test_full_join_keeps_both_sides():
    assert rows(
        "from employees e |> full join departments d on e.dept_id == d.dept_id"
        " |> select e.name as who, d.name as dept"
    ) == sorted_rows(*MATCHED, ("dan", None), (None, "ops"))


# --- each reference resolves to the side it names ---


def test_qualified_columns_come_from_their_own_side():
    assert rows(
        "from employees e |> join departments d on e.dept_id == d.dept_id"
        " |> select e.id as employee, d.id as department"
    ) == sorted_rows(("e1", "d1"), ("e2", "d2"), ("e3", "d1"))


def test_bare_column_from_one_side_only():
    assert rows(
        "from employees e |> join grades g on e.dept_id == g.code |> select e.name as who, label"
    ) == GRADED


def test_four_way_join_resolves_every_qualified_column():
    assert rows(
        "from employees e |> join departments d on e.dept_id == d.dept_id"
        " |> join projects p on p.dept_id == d.dept_id"
        " |> join grades g on g.code == d.dept_id"
        " |> select e.id as ei, d.id as di, p.id as pi, g.label as label"
    ) == sorted_rows(
        ("e1", "d1", "p1", "gold"),
        ("e2", "d2", "p2", "silver"),
        ("e3", "d1", "p1", "gold"),
    )


def test_self_join_keeps_the_two_sides_apart():
    assert rows(
        "from employees x |> join employees y on x.dept_id == y.dept_id"
        ' |> where x.name == "alice" |> select x.id as xi, y.id as yi'
    ) == sorted_rows(("e1", "e1"), ("e1", "e3"))


def test_qualified_rename_names_one_side():
    assert rows(
        "from employees e |> join departments d on e.dept_id == d.dept_id"
        " |> rename e.id as employee, d.id as department |> select employee, department"
    ) == sorted_rows(("e1", "d1"), ("e2", "d2"), ("e3", "d1"))


# --- `using` carries one copy of each key ---


def test_using_keeps_a_single_copy_of_the_key():
    assert rows(
        "from employees e |> join departments d using (dept_id)"
        " |> select dept_id, e.name as who"
    ) == sorted_rows((1, "alice"), (2, "bob"), (1, "carol"))


def test_either_side_reaches_the_carried_key():
    assert rows(
        "from employees e |> join departments d using (dept_id)"
        " |> select e.dept_id as left_key, d.dept_id as right_key"
    ) == sorted_rows((1, 1), (2, 2), (1, 1))


def test_using_with_several_keys():
    assert rows(
        "from employees a |> join employees b using (id, dept_id)"
        " |> select id, dept_id"
    ) == sorted_rows(("e1", 1), ("e2", 2), ("e3", 1), ("e4", 9))


def test_outer_join_using_preserves_unmatched_rows():
    assert rows(
        "from employees e |> left join departments d using (dept_id)"
        " |> select e.name as who, d.name as dept"
    ) == sorted_rows(*MATCHED, ("dan", None))


# --- later stages see the joined row ---


def test_stages_after_a_join():
    assert rows(
        "from employees e |> join departments d on e.dept_id == d.dept_id"
        " |> where e.salary > 100000"
        " |> extend e.salary / 12 as monthly"
        " |> select d.name as dept, monthly"
    ) == sorted_rows(("eng", 10000), ("eng", 20000))


def test_distinct_after_a_join():
    assert rows(
        "from employees e |> join departments d on e.dept_id == d.dept_id"
        " |> select d.name as dept |> distinct"
    ) == sorted_rows(("eng",), ("sales",))


def test_drop_after_a_join():
    assert rows(
        "from employees e |> join grades g on e.dept_id == g.code"
        " |> select e.name as who, label, code |> drop code"
    ) == GRADED
