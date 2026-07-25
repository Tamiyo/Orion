"""The shared schema and runner for the end-to-end query tests.

One schema serves every test. It is shaped so that mistakes show up in results
rather than hiding:

- `employees`, `departments` and `projects` share `id`, `name` and `dept_id`,
  so a bare reference to those is ambiguous and a qualified one has to pick a
  side. Their `id` values are prefixed per relation (`e1`, `d1`, `p1`), so a
  reference resolving to the wrong side is visible in the output.
- `grades` shares no column name with `employees`, which is the only way to
  join a relation that has no alias.
- `employees.dept_id` has a value (`9`) matching no department, and
  `departments` has one (`3`) matching no employee, so outer joins have
  something to preserve on either side.
- `employees.level` has a repeated value, so `distinct` has something to drop.

Needs a venv with the `yuzu` wheel (`maturin develop -m
crates/yuzu_python/Cargo.toml`) and `datafusion`.
"""

import yuzu
from datafusion import SessionContext, substrait

SCHEMA = """
struct Employee { id: str, name: str, dept_id: int64, level: int64, salary: int64, active: bool, rating: float64 }
table employees = Employee

struct Department { id: str, name: str, dept_id: int64 }
table departments = Department

struct Project { id: str, name: str, dept_id: int64 }
table projects = Project

struct Grade { code: int64, label: str }
table grades = Grade
"""

TABLES = {
    "employees": {
        "id": ["e1", "e2", "e3", "e4"],
        "name": ["alice", "bob", "carol", "dan"],
        "dept_id": [1, 2, 1, 9],
        "level": [1, 2, 3, 2],
        "salary": [120000, 90000, 240000, 60000],
        "active": [True, False, True, False],
        "rating": [1.5, 2.5, 3.5, 2.5],
    },
    "departments": {
        "id": ["d1", "d2", "d3"],
        "name": ["eng", "sales", "ops"],
        "dept_id": [1, 2, 3],
    },
    "projects": {
        "id": ["p1", "p2", "p3"],
        "name": ["apollo", "borealis", "cosmos"],
        "dept_id": [1, 2, 3],
    },
    "grades": {"code": [1, 2, 3], "label": ["gold", "silver", "bronze"]},
}


def order(row):
    """Sorts `None` last so outer-join results compare deterministically."""
    return tuple((value is None, value) for value in row)


def sorted_rows(*expected):
    return sorted(expected, key=order)


def rows(query, schema=SCHEMA, tables=None):
    """The query's result rows, order-independent."""
    ctx = SessionContext()
    for name, columns in (TABLES if tables is None else tables).items():
        ctx.from_pydict(columns, name=name)

    plan = substrait.Serde.deserialize_bytes(yuzu.compile(schema + query))
    logical = substrait.Consumer.from_substrait_plan(ctx, plan)
    collected = ctx.create_dataframe_from_logical_plan(logical).collect()
    return sorted(
        (
            tuple(row)
            for batch in collected
            for row in zip(*[column.to_pylist() for column in batch.columns])
        ),
        key=order,
    )


def error_of(query, schema=SCHEMA):
    """The first line of the compile error, or `None` if it compiles."""
    try:
        yuzu.compile(schema + query)
    except ValueError as failure:
        return str(failure).splitlines()[0]
    return None
