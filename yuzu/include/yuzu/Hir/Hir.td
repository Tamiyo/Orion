//===----------------------------------------------------------------------===//
// Yuzu HIR definitions.
//===----------------------------------------------------------------------===//
//
// Concrete HIR types for the Yuzu language. The schema (`Variant`,
// `Node`, `Child`, ...) is shared with the AST and lives in
// `yuzu/DSL/TreeBase.td`; this file only declares HIR instances of it.
// Consumed by `yuzu-tblgen -gen-hir-node-decls` to emit the C++ class
// hierarchy and by `-gen-hir-builder-decls` to emit the bump-allocated
// builder.
//
// Unlike the AST, HIR nodes drop the `SyntaxNode` backing and store
// their children as `const T *` pointers (and
// `llvm::ArrayRef<const T *>` for lists), allocated in a
// `llvm::BumpPtrAllocator` owned by the `HirBuilder`.
//
//===----------------------------------------------------------------------===//

include "yuzu/DSL/TreeBase.td"

//===----------------------------------------------------------------------===//
// Types
//===----------------------------------------------------------------------===//

def Bool : Native<"bool"> { let IsTrivial = 1; }
def Int64 : Native<"int64_t"> { let IsTrivial = 1; }
def Float64 : Native<"double"> { let IsTrivial = 1; }
// HIR strings are interned (`util::StringInterner`), so nodes hold a
// stable, trivially-copyable view rather than owning a `std::u32string`.
def String : Native<"std::u32string_view"> { let IsTrivial = 1; }
def OpRef : Native<"const yuzu::hir::Op *"> { let IsTrivial = 1; }

/// Universal HIR-node id. Declared as a schema Field so the generator
/// threads it through every constructor instead of special-casing it.
def Id : Native<"yuzu::hir::HirId"> { let IsTrivial = 1; }

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

//===----------------------------------------------------------------------===//
// Root
//===----------------------------------------------------------------------===//

/// Root of the HIR hierarchy. Emitted as a base class with a
/// `protected` constructor (so the root can't be constructed directly)
/// — every concrete HIR class transitively inherits from it.
/// `Namespace` is the C++ namespace the codegen wraps emitted types in.
def HirNode : Base {
    let Namespace = "yuzu::hir";
    let TreeName = "Hir";
    let Fields = (ins Custom<Id>:$id);
}


//===----------------------------------------------------------------------===//
// Base Nodes
//===----------------------------------------------------------------------===//

/// A bare identifier used as a *binding* (e.g. the `x` in `let x = ...`).
/// Sits outside `Expr` because declarations don't carry a type — the
/// type belongs to the value the binding gets initialised with, not to
/// the name itself. Use `IdentExpr` for identifier *references*.
def Ident : Node<HirNode> {
    let Fields = (ins Custom<String>:$name);
}

//===----------------------------------------------------------------------===//
// Expressions
//===----------------------------------------------------------------------===//

/// Top-level variant for anything that produces a value. The resolved
/// type lives in `HirContext`'s type side table (keyed by `HirId`), not
/// on the node — the tree stays immutable while the typer fills types in.
def Expr : Variant<HirNode> {}

/// An identifier *reference* — an identifier that appears in
/// expression position. Its type is resolved by name resolution, which
/// looks up the binding the identifier refers to.
def IdentExpr : Node<Expr> {
    let Fields = (ins Child<Ident>:$name);
}

/// A builtin operator applied to its arguments (`x + y`, `-x`). `op`
/// identifies the operator; the typer runs `op->resolve` to compute the
/// result type. Function calls are `FuncCallExpr`, a distinct node.
def CallExpr : Node<Expr> {
    let Fields = (ins
        Custom<OpRef>:$op,
        Children<Expr>:$args
    );
}

/// A function call `callee(args)`. The callee is the expression being
/// invoked (a function-typed value); the typer checks each argument against
/// the callee's parameter types and takes its return type.
def FuncCallExpr : Node<Expr> {
    let Fields = (ins
        Child<Expr>:$callee,
        Children<Expr>:$args
    );
}

//===----------------------------------------------------------------------===//
// Query expressions
//===----------------------------------------------------------------------===//
//
// A relation is an ordinary `Expr` of type `Relation[T]`. The pipe stages nest:
// `from X e |> select ...` lowers to `SelectExpr(FromExpr(X, e), [...])`.
// Relational correctness is a typing concern, not a distinct node tree.

/// The pipe source: `from <relation> [as] <alias>`. `alias` is null when omitted.
def FromExpr : Node<Expr> {
    let Fields = (ins
        Child<Ident>:$relation,
        Child<Ident>:$alias
    );
}

/// One projected column: the row expression and its optional output name.
def SelectItem : Node<HirNode> {
    let Fields = (ins
        Child<Expr>:$expr,
        Child<Ident>:$alias
    );
}

/// A `select` stage applied to an input relation.
def SelectExpr : Node<Expr> {
    let Fields = (ins
        Child<Expr>:$input,
        Children<SelectItem>:$items
    );
}

/// Field access: `base.field` (a struct field or a query row column).
def FieldAccessExpr : Node<Expr> {
    let Fields = (ins
        Child<Expr>:$base,
        Child<Ident>:$field
    );
}

/// One `name: value` initializer of a struct literal.
def StructLitField : Node<HirNode> {
    let Fields = (ins
        Child<Ident>:$name,
        Child<Expr>:$value
    );
}

/// A struct literal: `Employee { id: 1, name: "Bob" }`.
def StructLitExpr : Node<Expr> {
    let Fields = (ins
        Child<Ident>:$name,
        Children<StructLitField>:$fields
    );
}

/// A literal expression. Currently always a number; the literal value
/// is intentionally not stored here yet (a `Custom<Number>` field will
/// land once the literal type lives in the schema).
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
// types::Type expressions
//===----------------------------------------------------------------------===//

/// A written type annotation — the `int8` in `let x: int8 = …`. Mirrors
/// `ast::TypeAnnotation` so HIR stays self-contained: the typer resolves a
/// `TypeAnnotation` to a semantic `types::Type` instead of reaching back into the AST.
/// `NamedType` and `FuncType` are modeled; record type syntax is rejected
/// during lowering until the typer grows support.
def TypeAnnotation : Variant<HirNode> {}

/// A named type, optionally applied to type arguments (`Relation[T]`).
/// `args` is empty for a plain name.
def NamedTypeAnnotation : Node<TypeAnnotation> {
    let Fields = (ins
        Child<Ident>:$name,
        Children<TypeAnnotation>:$args
    );
}

/// A function type `(params...) -> result`, e.g. `(int32) -> int32`. Named
/// `FuncType` to avoid colliding with the semantic `FuncType` (in `types::Type.h`)
/// that it resolves to.
def FuncTypeAnnotation : Node<TypeAnnotation> {
    let Fields = (ins
        Children<TypeAnnotation>:$params,
        Child<TypeAnnotation>:$result
    );
}

//===----------------------------------------------------------------------===//
// Statements
//===----------------------------------------------------------------------===//

/// Top-level variant for statements.
def Stmt : Variant<HirNode> {}

/// Statement wrapping a single expression.
def ExprStmt : Node<Stmt> {
    let Fields = (ins
        Child<Expr>:$expr
    );
}

/// One `name: type` field of a struct.
def StructFieldDecl : Node<HirNode> {
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

/// A table registering a relation in the schema: `table employees = Employee`
/// (named) or `table events = { user_id: int32, ts: int64 }` (inline). Exactly
/// one of `rowStruct` / `inlineFields` is present.
def TableStmt : Node<Stmt> {
    let Fields = (ins
        Child<Ident>:$name,
        Child<Ident>:$rowStruct,
        Children<StructFieldDecl>:$inlineFields
    );
}

/// `let name [: type] = expr`. `annotation` is the written type, null when
/// the binding is inferred from its initializer. The resolved `types::Type` lives
/// in the `HirContext` type side table, keyed by this node's `HirId`.
def LetStmt : Node<Stmt> {
    let Fields =  (ins
        Child<Ident>:$name,
        Custom<Mutability>:$mutability,
        Child<TypeAnnotation>:$typeAnnotation,
        Child<Expr>:$expr
    );
}

/// `target = value`. The typer checks `target` is an assignable place bound
/// to a mutable `let`.
def AssignStmt : Node<Stmt> {
    let Fields = (ins
        Child<Expr>:$target,
        Child<Expr>:$value
    );
}

/// One `name: type` function parameter. `annotation` is the written type
/// (always present in well-formed source); the resolved `types::Type` lives in the
/// type side table keyed by this node's id.
def Param : Node<HirNode> {
    let Fields = (ins
        Child<Ident>:$name,
        Child<TypeAnnotation>:$typeAnnotation
    );
}

/// A brace-delimited statement sequence (a function body for now).
def BlockStmt : Node<Stmt> {
    let Fields = (ins
        Children<Stmt>:$stmts
    );
}

/// A return statement. The field `expr` is null for a bare `return` (unit).
def ReturnStmt : Node<Stmt> {
    let Fields = (ins
        Child<Expr>:$expr
    );
}

/// A trait named in a bound (`Add` in `where T: Add`).
def TraitRef : Node<HirNode> {
    let Fields = (ins Child<Ident>:$name);
}

/// One `where` clause: `subject: trait (+ trait)*`, e.g. `T: Add + Eq`.
def TypeBound : Node<HirNode> {
    let Fields = (ins
        Child<Ident>:$subject,
        Children<TraitRef>:$traits
    );
}

/// A function declaration. `bounds` is the `where` clause (empty if absent).
def FuncStmt : Node<Stmt> {
    let Fields = (ins
        Child<Ident>:$name,
        Children<Ident>:$typeParams,
        Children<Param>:$params,
        Child<TypeAnnotation>:$returnTypeAnnotation,
        Children<TypeBound>:$bounds,
        Child<BlockStmt>:$body
    );
}


//===----------------------------------------------------------------------===//
// Roots
//===----------------------------------------------------------------------===//

/// Top of an HIR tree: a flat sequence of statements.
def Root : Node<HirNode> {
    let Fields = (ins
        Children<Stmt>:$stmts
    );
}
