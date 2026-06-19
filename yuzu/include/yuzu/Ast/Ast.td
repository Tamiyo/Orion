//===----------------------------------------------------------------------===//
// Yuzu AST definitions.
//===----------------------------------------------------------------------===//
//
// Concrete AST types for the Yuzu language. The schema (`Variant`, `Node`,
// `Child`, ...) lives in `yuzu/DSL/TreeBase.td`; this file only declares
// instances of it. Consumed by `yuzu-tblgen -gen-ast-node-decls` to emit
// the C++ class hierarchy under `Expr.h.inc`.
//
//===----------------------------------------------------------------------===//

include "yuzu/DSL/TreeBase.td"

//===----------------------------------------------------------------------===//
// Types
//===----------------------------------------------------------------------===//

def Bool : Native<"bool"> { let IsTrivial = 1; }
def Int64 : Native<"int64_t"> { let IsTrivial = 1; }
def Float64 : Native<"double"> { let IsTrivial = 1; }
def String : Native<"std::u32string"> { let ViewName = "std::u32string_view"; }

def Visibility : Enum<"uint8_t"> {
    let Cases = [
        EnumCase<"Public">,
        EnumCase<"Protected">,
        EnumCase<"Internal">,
        EnumCase<"Private">,
    ];
}

def Mutability : Enum<"uint8_t"> {
    let Cases = [
        EnumCase<"Mutable">,
        EnumCase<"Immutable">,
    ];
}

/// A binary operator.
def BinOp : Enum<"uint8_t"> {
    let Cases = [
        // Logical
        EnumCase<"And">,
        EnumCase<"Or">,
        EnumCase<"In">,
        EnumCase<"NotIn">,

        // Arith
        EnumCase<"Add">,
        EnumCase<"Sub">,
        EnumCase<"Mul">,
        EnumCase<"Div">,
        EnumCase<"Pow">,

        // Comparison
        EnumCase<"Eq">,
        EnumCase<"Neq">,
        EnumCase<"Lt">,
        EnumCase<"Lte">,
        EnumCase<"Gt">,
        EnumCase<"Gte">,

        // Bits
        EnumCase<"ShiftLeft">,
        EnumCase<"ShiftRight">,
    ];
}

/// A unary operator.
def UnaryOp : Enum<"uint8_t"> {
    let Cases = [
        EnumCase<"Neg">,
        EnumCase<"Pos">,
        EnumCase<"Not">,
    ];
}

//===----------------------------------------------------------------------===//
// Root
//===----------------------------------------------------------------------===//

/// Root of the AST hierarchy. Emitted as a base class with a `protected`
/// constructor (so the root can't be constructed directly) — every concrete
/// AST class transitively inherits from it. `Namespace` is the C++
/// namespace the codegen wraps emitted types in.
def AstNode : Base {
    let Namespace = "yuzu::ast";
}

//===----------------------------------------------------------------------===//
// Base Nodes
//===----------------------------------------------------------------------===//

/// A bare identifier used as a *binding* (e.g. the `x` in `let x = ...`).
/// Carries no type — declarations don't have one until they're tied to
/// an initialiser. For identifier *references* (uses) see `IdentExpr`.
def Ident : Node<AstNode> {
    let Fields = (ins Custom<String>:$name);
}

//===----------------------------------------------------------------------===//
// Expressions
//===----------------------------------------------------------------------===//

/// Top-level variant for anything that produces a value.
def Expr : Variant<AstNode> {}

/// An identifier *reference* — an identifier that appears in
/// expression position (e.g. the `x` in `x + 1`). Resolves to whatever
/// binding it refers to and inherits its type from there.
def IdentExpr : Node<Expr> {
    let Fields = (ins Child<Ident>:$name);
}

/// Binary infix expression like `a + b`. The operator token isn't carried
/// in the AST — recover it by inspecting the underlying syntax tree.
def BinaryExpr : Node<Expr> {
    let Fields = (ins
        Child<Expr>:$lhs,
        Custom<BinOp>:$op,
        Child<Expr>:$rhs
    );
}

def UnaryExpr : Node<Expr> {
    let Fields = (ins
        Custom<UnaryOp>:$op,
        Child<Expr>:$expr
    );
}

/// The parenthesized argument list of a call: `(a, b, c)`. Wraps the args
/// in their own node so they don't collide with the callee — both are
/// `Expr`, and a bare `Children<Expr>` on `CallExpr` would otherwise sweep
/// the callee in too.
def ArgList : Node<AstNode> {
    let Fields = (ins Children<Expr>:$args);
}

/// A function call `callee(args)`. `callee` is any expression (a bare name
/// today; `getFunc()(x)` and methods fall out for free). Operators are not
/// calls here — they stay `BinaryExpr`/`UnaryExpr`.
def CallExpr : Node<Expr> {
    let Fields = (ins
        Child<Expr>:$callee,
        Child<ArgList>:$args
    );
}

/// Parenthesized expression like `(a + b)`. Purely a grouping wrapper —
/// it holds the inner expression and exists so the `(`/`)` tokens nest
/// under one node instead of leaking into an enclosing expression. Lowers
/// to its inner expression and contributes no HIR of its own.
def ParenExpr : Node<Expr> {
    let Fields = (ins Child<Expr>:$expr);
}

//===----------------------------------------------------------------------===//
// Query expressions
//===----------------------------------------------------------------------===//
//
// A relation is an ordinary expression of type `Relation[T]`, written in pipe
// syntax: `from <rel> [as] <alias> |> select <expr> [as] <alias>, ...`. The
// stages nest — `from X e |> select ...` parses to `SelectExpr(FromExpr(X,e))`.

/// The pipe source: `from <relation> [as] <alias>`. `alias` is optional.
def FromExpr : Node<Expr> {
    let Fields = (ins
        Child<Ident>:$relation,
        Child<Ident>:$alias
    );
}

/// One projected column: `<expr> [as <alias>]`. `alias` is optional.
def SelectItem : Node<AstNode> {
    let Fields = (ins
        Child<Expr>:$expr,
        Child<Ident>:$alias
    );
}

/// A `|> select <items>` stage applied to an input relation.
def SelectExpr : Node<Expr> {
    let Fields = (ins
        Child<Expr>:$input,
        Children<SelectItem>:$items
    );
}

/// A `|> where <predicate>` stage: keep only the rows the predicate accepts.
def WhereExpr : Node<Expr> {
    let Fields = (ins
        Child<Expr>:$input,
        Child<Expr>:$predicate
    );
}

/// A `|> distinct` stage: remove duplicate rows. Same row type as the input.
def DistinctExpr : Node<Expr> {
    let Fields = (ins Child<Expr>:$input);
}

/// Field access: `base.field` (e.g. a struct field or a query row column).
def FieldAccessExpr : Node<Expr> {
    let Fields = (ins
        Child<Expr>:$base,
        Child<Ident>:$field
    );
}

/// One `name: value` field initializer of a struct expression.
def StructFieldInit : Node<AstNode> {
    let Fields = (ins
        Child<Ident>:$name,
        Child<Expr>:$value
    );
}

/// A struct expression: `Employee { id: 1, name: "Bob" }`. `name` is the struct
/// type; each field initializes a declared field by name.
def StructExpr : Node<Expr> {
    let Fields = (ins
        Child<Ident>:$name,
        Children<StructFieldInit>:$fields
    );
}

/// A literal expression. Currently always a number; the literal value lives
/// on the syntax token, not in the AST.
def Literal : Variant<Expr> {}

def BoolLit : Node<Literal> {
     let Fields = (ins
        Custom<Bool>:$value
    );
}

def IntLit : Node<Literal> {
     let Fields = (ins
        Custom<Int64>:$value
    );
}

def FloatLit : Node<Literal> {
     let Fields = (ins
        Custom<Float64>:$value
    );
}

def StringLit : Node<Literal> {
     let Fields = (ins
        Custom<String>:$value,
        Custom<Bool>:$isRaw
    );
}

//===----------------------------------------------------------------------===//
// Type expressions
//===----------------------------------------------------------------------===//

/// A type as written in *source* — a syntactic annotation, distinct from
/// the semantic `hir::Type` it eventually lowers to. A variant so the
/// different shapes (named, function, record) share one field slot, e.g.
/// `StructField.type`.
def TypeAnnotation : Variant<AstNode> {}

/// A named type, optionally applied to type arguments:
/// `string`, `Employee`, `Relation[Employee]`, `List[Relation[T]]`. The
/// `args` list is empty for a plain name; each argument is itself a
/// `TypeAnnotation`, so nesting falls out for free. Value-parameterized types
/// like `decimal(2, 12)` are a later addition.
def NamedTypeAnnotation : Node<TypeAnnotation> {
    let Fields = (ins
        Child<Ident>:$name,
        Children<TypeAnnotation>:$args
    );
}

/// The parenthesized parameter list of a function type: `(int, str)`. Wraps
/// the params in their own node so they don't collide with the result — both
/// are `TypeAnnotation`, and a bare `Children<TypeAnnotation>` on `FuncType`
/// would otherwise sweep the result in too (cf. `ArgList`).
def FuncTypeAnnotationParams : Node<AstNode> {
    let Fields = (ins Children<TypeAnnotation>:$params);
}

/// A function type: `(int, str) -> bool`. Positional parameter types and a
/// single result; `params` is empty for `() -> T`.
def FuncTypeAnnotation : Node<TypeAnnotation> {
    let Fields = (ins
        Child<FuncTypeAnnotationParams>:$params,
        Child<TypeAnnotation>:$result
    );
}

/// One `name: type` member of a `RecordType`. Unlike `StructField` it
/// carries no visibility — anonymous records have no access control.
def RecordField : Node<AstNode> {
    let Fields = (ins
        Child<Ident>:$name,
        Child<TypeAnnotation>:$type
    );
}

/// An anonymous record type: `(sum: decimal, count: int64)`. Structural
/// (no declared name) — used for aggregate accumulators and `select`
/// output rows.
def RecordType : Node<TypeAnnotation> {
    let Fields = (ins
        Children<RecordField>:$fields
    );
}

//===----------------------------------------------------------------------===//
// Statements
//===----------------------------------------------------------------------===//

/// Top-level variant for statements.
def Stmt : Variant<AstNode> {}

/// One `name: type` field of a struct. (Visibility / trait bounds are
/// deferred — re-added when the access-control design lands.)
def StructFieldDecl : Node<AstNode> {
    let Fields = (ins
        Child<Ident>:$name,
        Child<TypeAnnotation>:$type
    );
}

/// A nominal struct declaration: `struct Employee { id: int32, name: str }`.
def StructStmt : Node<Stmt> {
    let Fields = (ins
        Child<Ident>:$name,
        Children<StructFieldDecl>:$fields
    );
}

/// A table declaration registering a relation in the schema:
/// `table employees = Employee` (named struct rows) or
/// `table events = { user_id: int32, ts: int64 }` (inline rows). Exactly one of
/// `rowStruct` / `inlineFields` is present. `name` is the table; `rowStruct` (a
/// Child<Ident>) is positionally second so the table name always wins index 0.
def TableStmt : Node<Stmt> {
    let Fields = (ins
        Child<Ident>:$name,
        Child<Ident>:$rowStruct,
        Children<StructFieldDecl>:$inlineFields
    );
}

/// Statement wrapping a single expression: `42`, `a + b`, etc.
def ExprStmt : Node<Stmt> {
    let Fields = (ins
        Child<Expr>:$expr
    );
}

def LetStmt : Node<Stmt> {
    let Fields = (ins
        Child<Ident>:$name,
        Custom<Mutability>:$mutability, 
        Child<TypeAnnotation>:$typeAnnotation, // optional — present only when annotated
        Child<Expr>:$expr
    );
}

/// An assignment `target = value`. `target` is any expression; the typer
/// checks it's an assignable (mutable) place. Distinct from `let`, which
/// introduces a binding.
def AssignStmt : Node<Stmt> {
    let Fields = (ins
        Child<Expr>:$target,
        Child<Expr>:$value
    );
}

/// A generic type parameter at a declaration site: the `T` in `fn id[T]`.
/// Bounds are written separately in the `where` clause.
def TypeParam : Node<AstNode> {
    let Fields = (ins
        Child<Ident>:$name
    );
}

/// A trait named in a bound, e.g. the `Add` in `where T: Add`. Wraps an
/// `Ident` in its own node so a bound's trait list doesn't collide with its
/// subject (both would otherwise be `Ident` children).
def TraitRef : Node<AstNode> {
    let Fields = (ins
        Child<Ident>:$name
    );
}

/// One clause of a `where`: `subject: trait (+ trait)*`, e.g. `T: Add + Eq`.
def TypeBound : Node<AstNode> {
    let Fields = (ins
        Child<Ident>:$subject,
        Children<TraitRef>:$traits
    );
}

/// One `name: type` function parameter.
def Param : Node<AstNode> {
    let Fields = (ins
        Child<Ident>:$name,
        Child<TypeAnnotation>:$type
    );
}

/// A brace-delimited statement sequence: a function body, eventually any
/// block. `{ let x = 1\n return x }`.
def BlockStmt : Node<Stmt> {
    let Fields = (ins
        Children<Stmt>:$stmts
    );
}

/// `return expr` — `expr` is optional (`return` alone returns unit).
def ReturnStmt : Node<Stmt> {
    let Fields = (ins
        Child<Expr>:$expr
    );
}

/// A function declaration: `fn id[T](x: T) -> T where T: Add { ... }`.
/// `typeParams` is empty for a non-generic function; `result` is the optional
/// return annotation; `bounds` is the optional `where` clause.
def FuncStmt : Node<Stmt> {
    let Fields = (ins
        Child<Ident>:$name,
        Children<TypeParam>:$typeParams,
        Children<Param>:$params,
        Child<TypeAnnotation>:$result, // optional
        Children<TypeBound>:$bounds,
        Child<BlockStmt>:$body
    );
}

//===----------------------------------------------------------------------===//
// Roots
//===----------------------------------------------------------------------===//

/// Top-level variant for statements. No concrete subtypes yet; the codegen
/// emits a placeholder `class Stmt {};` until one shows up.
def Root : Node<AstNode> {
    let Fields = (ins
        Children<Stmt>:$stmts
    );
}
