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

def Bool : Native<"bool"> {}
def Int64 : Native<"int64_t"> {}
def Float64 : Native<"double"> {}
def String : Native<"std::u32string"> {}
def OpRef : Native<"const yuzu::hir::Op *"> {}

/// Universal HIR-node id. Declared as a schema Field so the generator
/// threads it through every constructor instead of special-casing it.
def Id : Native<"yuzu::hir::HirId"> {}

/// Pointer to an interned `Type`. Stored on every `Expr` so the tree is
/// typed by construction.
def TypeRef : Native<"const yuzu::hir::Type *"> {}

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
// Expressions
//===----------------------------------------------------------------------===//

/// Top-level variant for anything that produces a value. Every Expr
/// carries the type it resolves to; the typer fills this in during
/// lowering so downstream passes consume an already-typed tree.
def Expr : Variant<HirNode> {
    let Fields = (ins Custom<TypeRef>:$type);
}

/// A 'call' expression to a set of oeprators and some arguments.
def CallExpr : Node<Expr> {
    let Fields = (ins
        Custom<OpRef>:$op,
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

//===----------------------------------------------------------------------===//
// Roots
//===----------------------------------------------------------------------===//

/// Top of an HIR tree: a flat sequence of statements.
def Root : Node<HirNode> {
    let Fields = (ins
        Children<Stmt>:$stmts
    );
}
