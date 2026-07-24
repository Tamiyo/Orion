import yuzu
from datafusion import SessionContext
from datafusion import substrait

QUERY = """
struct Employee { id: str, department: str, salary: int64 }
table employees = Employee

fn get_threshold() -> int64 {
    let x = 2
    let mut y = 250000
    y = 50000
    return x * y
}

from employees e
|> where e.salary > get_threshold()
|> extend e.salary / 12 as monthly
|> select e.id, e.department, monthly
"""

ctx = SessionContext()
ctx.from_pydict(
    {
        "id": ["alice", "bob", "carol", "dan"],
        "department": ["eng", "sales", "eng", "support"],
        "salary": [120000, 90000, 240000, 60000],
    },
    name="employees",
)

compile_opts = yuzu.CompileOptions(
    debug_anf=True,
    debug_reduce=True,
    debug_substrait=True,
)

plan = substrait.Serde.deserialize_bytes(yuzu.compile(QUERY, compile_opts))
logical = substrait.Consumer.from_substrait_plan(ctx, plan)
ctx.create_dataframe_from_logical_plan(logical).show()