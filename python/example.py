import yuzu
from datafusion import SessionContext
from datafusion import substrait

QUERY = """
struct Employee { id: str, department: str, salary: int64 }
table employees = Employee

struct Student { id: str, major: str }
table students = Student

fn get_threshold() -> int64 {
    let x = 2
    let mut y = 250000
    y = 50000
    return x * y
}

from employees e
|> join students s on e.id == s.id
|> rename e.id as eid, s.id as sid
|> where e.salary > get_threshold()
|> extend e.salary / 12 as monthly
|> select eid, e.department, monthly, major
"""

ctx = SessionContext()
ctx.from_pydict(
    {
        "id": ["alice", "bob", "carol", "dan"],
        "department": ["eng", "sales", "eng", "support"],
        "salary": [120000, 90000, 240000, 240000],
    },
    name="employees",
)

ctx.from_pydict(
    {
        "id": ["alan", "bob", "jeff", "dan"],
        "major": ["eng", "history", "english", "math"],
    },
    name="students",
)


compile_opts = yuzu.CompileOptions()
plan = substrait.Serde.deserialize_bytes(yuzu.compile(QUERY, compile_opts))
logical = substrait.Consumer.from_substrait_plan(ctx, plan)
ctx.create_dataframe_from_logical_plan(logical).show()