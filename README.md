# Yuzu

Yuzu is a small, statically-typed **relational query language**. You write
queries with pipe syntax (`|>`), and Yuzu compiles them to
[Substrait](https://substrait.io) — a portable query-plan format. Because the
output is Substrait, a Yuzu query can run on any engine that consumes it; today
we execute plans with DuckDB.

> **Status:** Yuzu is early and under active development. The pipe operators,
> custom traits, and trait bounds described below are all works in progress and
> not yet stable.

```
struct Employee { id: str, department: str, salary: int64 }
table employees = Employee

from employees e
|> where e.salary > 100000
|> extend e.salary / 12 as monthly
|> select e.id, e.department, monthly
```

A query is an ordinary expression of type `Relation[Row]`. Each `|>` stage
transforms the relation flowing through it, and only sees the columns of its
immediate input — just like SQL's pipe syntax.

## Language tour

### Structs and tables

```
struct Employee { id: str, department: str, salary: int64 }
table employees = Employee
```

A `struct` is a named row type; a `table` registers a relation you can query
with `from`.

### Pipe operators (WIP)

```
from employees e
|> where e.salary > 100000      // keep rows matching a predicate
|> extend e.salary * 2 as bonus // add computed columns
|> rename department as dept    // rename columns
|> drop id                      // remove columns
|> select dept, bonus           // project columns
|> distinct                     // remove duplicate rows
```

| operator | effect |
|---|---|
| `from t e` | the pipe source (the alias `e` is optional) |
| `select …` | project a new set of columns |
| `where p` | keep rows where the boolean predicate `p` holds |
| `extend e as n` | append computed columns, keeping the existing ones |
| `drop a, b` | remove named columns |
| `rename a as b` | rename columns |
| `distinct` | drop duplicate rows |

Columns are referenced by bare name against the current stage's row (`salary`),
or through the source alias (`e.salary`). The set of operators and their
semantics are still evolving.

### Functions

```
fn bonus(base: int64) -> int64 {
    return base * 2
}

from employees e |> select bonus(e.salary) as bonus
```

Functions can be generic, with trait bounds:

```
fn add[T](a: T, b: T) -> T where T: Add {
    return a + b
}
```

> **WIP:** custom traits and the builtin trait bounds (`where T: Add`, …) are a
> work in progress and not yet stable.

Calls to known functions are inlined and constant-folded at compile time, so
`bonus(e.salary)` lowers to `e.salary * 2` in the emitted plan.

### Values and lists

```
let limit = 10 + 5
let ids: List[int32] = [1, 2, 3]
```

Primitive types are the fixed- and unbounded-width integers (`int8`…`int64`,
`uint8`…`uint64`), floats (`float32`, `float64`), `bool`, and `str`, plus
`List[T]`, structs, and relations.

## How it works

A Yuzu program flows through a fixed pipeline:

```
source → tokens → syntax tree → HIR (typed) → ANF (reduced) → Substrait plan
```

- **HIR** is the typed intermediate representation; type inference, generics,
  and pipe-stage resolution happen here.
- **ANF** is a normalized form on which a demand-driven partial evaluator runs:
  constant folding, copy/constant propagation, function inlining, and
  dead-function elimination — so the emitted plan contains only the reduced
  query.
- The **Substrait** backend emits a portable JSON plan. Any Substrait-compatible
  engine can execute it; we currently run it on DuckDB via `from_substrait_json`.

## Roadmap

A guiding goal is to make relational plans **first-class, inspectable values**:

- **Relational graphs** — expose a query's plan as a graph you can traverse in
  Yuzu code, not just compile away.
- **First-class transformation APIs** — operate on relational plans directly
  (match, rewrite, inject stages), so passes like privacy transforms
  (e.g. k-anonymity filters over quasi-identifiers) can be written in Yuzu
  itself rather than in the compiler.

## Building and running

The build wraps CMake + Ninja:

```
make build                              # build everything
make test                               # run the test suite
make compile FILE=path/to/query.yz      # compile one file
```

Useful flags pass through `ARGS`:

```
make compile FILE=q.yz ARGS=--artifacts=out   # write out/plan.substrait.json
make compile FILE=q.yz ARGS=--debug-hir       # dump the typed HIR
make compile FILE=q.yz ARGS=--time-passes     # per-pass compile timings
```

There's also an interactive REPL (`make repl`) and Python bindings for invoking
the compiler from Python.
