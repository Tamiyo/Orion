# Compile strategy: lowering Yuzu to Substrait

How Yuzu compiles its imperative-plus-relational source to an engine-agnostic
query plan, and the decisions behind it. This is the source of truth; the
TCF-decoder write-up and architecture sketches feed into it but defer here.

## Target: Substrait, single backend

We emit **Substrait** protobuf plans and execute them on DuckDB's substrait
extension. SQL is only a *readable mental model* for plans during development —
we never emit SQL text. Tests assert on **DuckDB execution results**, using
DuckDB's own `get_substrait_json('SELECT …')` as the ground-truth reference.

Verified empirically (see [[substrait-backend-constraints]]):
- DuckDB substrait extension has **no linux_arm64 build past DuckDB 1.2.2** — we
  pin to `duckdb==1.2.2` on arm64. Calls are table functions:
  `get_substrait_json(?)` / `from_substrait_json(?)`.
- Vendored Substrait protos are pinned to commit `af532c7` (the DuckDB 1.2.x
  submodule), which uses `extension_uris` (not the renamed `extension_urns`).
- These expression nodes round-trip AND execute on DuckDB's consumer:
  `scalarFunction`, `literal`, `selection`, `cast`, `ifThen`, `singularOrList`.
  There is **no iteration node**. DuckDB lowers `CASE a WHEN` to `ifThen`, so we
  canonicalize all branching to `ifThen` and never emit `switchExpression`.

## IR pipeline: AST → HIR → ANF → Substrait

| IR | Role | Typing |
|----|------|--------|
| **AST** | syntax tree (green/red) | untyped |
| **HIR** | resolved, type-inferred tree | inference subsystem fills a `HirId→Type` side table |
| **ANF** | A-normal form; the optimization + pre-emit IR | **typed by construction** — types carried from HIR, maintained as ANF simplifies. No re-inference. |
| **Substrait** | emitted protobuf plan | — |

**ANF is a new tree** (its own `Anf.td` via the shared `TreeBase.td` DSL,
its own arena/builder/visitor/printer), lowered from HIR by a pass analogous to
`HirLowerer` (AST→HIR). It is **relational ops carrying ANF scalar
let-sequences** — effectively a typed, in-memory mirror of Substrait, so the
emitter is a near-mechanical ANF→protobuf walk. Sharing is expressed the only
way Substrait allows: a multi-use `let` becomes a **projection column**; a
single-use `let` inlines into the expression tree.

## Type system: shared `yuzu::types`

Type *definitions* are IR-agnostic and shared by HIR and ANF; type *inference*
is HIR-specific.

- **Moves out → `yuzu::types`** (`yuzu/Types/`): `Type.h` (the lattice +
  `TypeKind` + concrete `*Type`), `TypeFactory.h` (interner). HIR and ANF share
  **one `TypeFactory` instance** so interned type identity (`==` on `Type*`)
  holds across both IRs.
- **Stays in HIR** (its typing subsystem): `TypeResolver`, `TypeInferrer`,
  `TypeConcretizer`, `TypeCoercion`, `TypeUnifier`, and `TypeContext` (the
  `HirId→Type` side table + trait registry). ANF never re-runs these.
- **ANF attaches types on-node** (a `Type*` field), since types are known at
  construction — no side table, no context needed to ask a value's type.

## Lowering HIR → ANF (the normalizer)

Every valid program must reduce to a pure relational-algebra DAG — no runtime
UDF boundary. Restrictions enforce this: **loops must have compile-time-constant
bounds (statically unrolled); data-dependent loops are rejected.**

- **Inline** user-function calls — keep the body's `let`s (don't substitute and
  duplicate); rename for last-write-wins SSA → lands in ANF.
- **Constant-fold** builtin ops over literals.
- *(future)* **Functionalize structured control flow**: `if/else` → `ifThen`;
  static `for` → unrolled. The merge value at a branch is the φ → `ifThen`.
- *(future)* **Monomorphize** generics; **comptime-evaluate** static state
  (the bit-reader: `Reader`/offsets are static, bytes are dynamic; partial-
  evaluate the static part away, leaving the dynamic residual expression).

## Optimizer over ANF

Uniform flat-list rewrites on named bindings: constant fold, copy propagation,
DCE, and **CSE by RHS equality** (identical right-hand sides collapse to one
binding → one projection column).

## Decisions on record

- **Own IR, not MLIR.** Target is an expression *tree* (no SSA/blocks) and the
  language is short-pipelined; MLIR's SSA framework is an impedance mismatch at
  the Substrait boundary and replaces working infrastructure. Revisit only at a
  real trigger: a **second backend** (e.g. native UDF codegen) or the optimizer
  becoming painful to maintain by hand.
- **ANF is a representation, not a framework.** It gives the SSA-like
  optimization substrate cheaply in our own IR — which is exactly why it does
  *not* argue for MLIR.
- **LLVM reserved** for the eventual native-UDF path (data-dependent loops →
  vectorized native code), not for the IR/transform framework.

## Milestones

- **M0 (now):** extract `yuzu::types`; scaffold the ANF tree.
- **M1:** HIR→ANF straight-line inline + fold; relational emitter (read +
  project); execute on DuckDB 1.2.2 with a golden/round-trip test.
- **M2:** control flow (`if/for/while`) → `ifThen` + static unroll;
  functionalization.
- **M3:** generics monomorphization; struct handling (keep-as-STRUCT vs
  decompose-to-columns); comptime evaluation; CSE → projection columns.
