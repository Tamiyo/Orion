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

//===----------------------------------------------------------------------===//
// Root
//===----------------------------------------------------------------------===//

/// Root of the HIR hierarchy. Emitted as a base class with a
/// `protected` constructor (so the root can't be constructed directly)
/// — every concrete HIR class transitively inherits from it.
/// `Namespace` is the C++ namespace the codegen wraps emitted types in.
def HirNode : Base {
    let Namespace = "yuzu::hir";
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
/// result type. Function calls are `FnCallExpr`, a distinct node.
def CallExpr : Node<Expr> {
    let Fields = (ins
        Custom<OpRef>:$op,
        Children<Expr>:$args
    );
}

/// A function call `callee(args)`. The callee is the expression being
/// invoked (a function-typed value); the typer checks each argument against
/// the callee's parameter types and takes its return type.
def FnCallExpr : Node<Expr> {
    let Fields = (ins
        Child<Expr>:$callee,
        Children<Expr>:$args
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

/// The binding's resolved type — the annotation when present (and checked
/// against the initializer), otherwise inferred from it — lives in the
/// `HirContext` type side table, keyed by this node's `HirId`.
def LetStmt : Node<Stmt> {
    let Fields =  (ins
        Child<Ident>:$name,
        Child<Expr>:$expr
    );
}

/// One `name: type` function parameter. The parameter's type (lowered from
/// its annotation) lives in the type side table keyed by this node's id.
def Param : Node<HirNode> {
    let Fields = (ins
        Child<Ident>:$name
    );
}

/// A brace-delimited statement sequence (a function body for now).
def BlockStmt : Node<Stmt> {
    let Fields = (ins
        Children<Stmt>:$stmts
    );
}

/// `return expr` — `expr` is null for a bare `return` (unit).
def ReturnStmt : Node<Stmt> {
    let Fields = (ins
        Child<Expr>:$expr
    );
}

/// A function declaration. `typeParams` are the generic parameters (bare
/// names for now); each parameter's and the function's signature type live
/// in the type side table. The declared return annotation, if any, is
/// stored at this node's id until the typer replaces it with the full
/// signature type.
def FnStmt : Node<Stmt> {
    let Fields = (ins
        Child<Ident>:$name,
        Children<Ident>:$typeParams,
        Children<Param>:$params,
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
