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

/// Parenthesized expression like `(a + b)`. Purely a grouping wrapper —
/// it holds the inner expression and exists so the `(`/`)` tokens nest
/// under one node instead of leaking into an enclosing expression. Lowers
/// to its inner expression and contributes no HIR of its own.
def ParenExpr : Node<Expr> {
    let Fields = (ins Child<Expr>:$expr);
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
def TypeExpr : Variant<AstNode> {}

/// A named type, optionally applied to type arguments:
/// `string`, `Employee`, `Relation[Employee]`, `List[Relation[T]]`. The
/// `args` list is empty for a plain name; each argument is itself a
/// `TypeExpr`, so nesting falls out for free. Value-parameterized types
/// like `decimal(2, 12)` are a later addition.
def NamedType : Node<TypeExpr> {
    let Fields = (ins
        Child<Ident>:$name,
        Children<TypeExpr>:$args
    );
}

/// A function type: `(int, str) -> bool`. Positional parameter types and a
/// single result; `params` is empty for `() -> T`.
def FuncType : Node<TypeExpr> {
    let Fields = (ins
        Children<TypeExpr>:$params,
        Child<TypeExpr>:$result
    );
}

/// One `name: type` member of a `RecordType`. Unlike `StructField` it
/// carries no visibility — anonymous records have no access control.
def RecordField : Node<AstNode> {
    let Fields = (ins
        Child<Ident>:$name,
        Child<TypeExpr>:$type
    );
}

/// An anonymous record type: `(sum: decimal, count: int64)`. Structural
/// (no declared name) — used for aggregate accumulators and `select`
/// output rows.
def RecordType : Node<TypeExpr> {
    let Fields = (ins
        Children<RecordField>:$fields
    );
}

//===----------------------------------------------------------------------===//
// Statements
//===----------------------------------------------------------------------===//

/// Top-level variant for statements.
def Stmt : Variant<AstNode> {}

def StructField : Node<AstNode> {
    let Fields = (ins
        Custom<Visibility>:$visibility,
        Child<Ident>:$name,
        Child<TypeExpr>:$type
    );
}

def StructStmt : Node<Stmt> {
    let Fields = (ins
        Custom<Visibility>:$visibility,
        Child<Ident>:$bound, // optional
        Child<Ident>:$name,
        Children<StructField>:$fields
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
        Child<TypeExpr>:$type, // optional — present only when annotated
        Child<Expr>:$expr
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
