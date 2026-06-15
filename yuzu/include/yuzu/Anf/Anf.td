//===----------------------------------------------------------------------===//
// Yuzu ANF (A-normal form) definitions.
//===----------------------------------------------------------------------===//
//
// The typed form HIR lowers to. An `Atom` is a value that is already there (a
// name or a literal); every other `Expr` is one computation whose operands are
// all `Atom`s. Intermediate computations are named by `LetStmt`s — each wrapping
// a `Binding` — in a `BlockStmt`, a flat sequence where each binding scopes over
// the statements after it, and a `VarAtom` use points straight back at its
// `Binding`. Mutation becomes SSA `LetStmt`s in the lowerer; only `ReturnStmt`
// carries control flow. Types ride on each `Expr`; metadata HIR already enforced
// (visibility, mutability) is dropped.
//
// A separate beta-substitution pass inlines `FuncCallExpr`s; the lowerer never
// does.
//
// Schema shared with AST/HIR in `yuzu/DSL/TreeBase.td`; `TreeName = "Anf"`.
//
//===----------------------------------------------------------------------===//

include "yuzu/DSL/TreeBase.td"

//===----------------------------------------------------------------------===//
// Types
//===----------------------------------------------------------------------===//

def Bool : Native<"bool"> { let IsTrivial = 1; }
def Int64 : Native<"int64_t"> { let IsTrivial = 1; }
def Float64 : Native<"double"> { let IsTrivial = 1; }
def String : Native<"std::u32string_view"> { let IsTrivial = 1; }

/// Resolved semantic type, carried on every `Expr`.
def TypeRef : Native<"const yuzu::types::Type *"> { let IsTrivial = 1; }

/// Builtin operator descriptor (ANF's own; see `Anf/Ops/BuiltinOps.h`).
def OpRef : Native<"const yuzu::anf::Op *"> { let IsTrivial = 1; }

/// A non-owning back-edge from a `VarAtom` use to the `Binding` it reads. The
/// binding is owned by its block / param list, not by the use.
def BindingRef : Native<"const yuzu::anf::Binding *"> { let IsTrivial = 1; }

/// A non-owning reference from a `FuncRef` to the `FuncStmt` it names. The
/// function is owned by the root, not by the reference.
def FuncStmtRef : Native<"const yuzu::anf::FuncStmt *"> { let IsTrivial = 1; }

/// Universal ANF-node id; threaded through every constructor by the generator.
def Id : Native<"yuzu::anf::AnfId"> { let IsTrivial = 1; }

//===----------------------------------------------------------------------===//
// Root of the hierarchy
//===----------------------------------------------------------------------===//

def AnfNode : Base {
    let Namespace = "yuzu::anf";
    let TreeName = "Anf";
    let Mutable = 1;
    let Fields = (ins Custom<Id>:$id);
}

/// A bare name in a naming position (a binding, field, column, or table).
def Ident : Node<AnfNode> {
    let Fields = (ins Custom<String>:$name);
}

/// The two top categories, declared up front so each may refer to the other:
/// an `Expr` produces a value; a `Stmt` binds, controls, or declares.
def Expr : Variant<AnfNode> {}
def Stmt : Variant<AnfNode> {}

//===----------------------------------------------------------------------===//
// Expressions. An `Expr` produces a value; the `Atom` subset is trivial, and
// every non-`Atom` expression takes only `Atom` operands.
//===----------------------------------------------------------------------===//

def Atom : Variant<Expr> {}

/// Reads a binding as a value — a direct back-edge to its `Binding`, so
/// resolution is a pointer chase, not a name lookup.
def VarAtom : Node<Atom> {
    let Fields = (ins
        Custom<BindingRef>:$binding,
        Custom<TypeRef>:$type
    );
}

/// A function named as a value — the resolved target of a direct call. Inlining
/// chases `func` to the body; an indirect call keeps a `VarAtom` callee. The
/// reference is non-owning (the `FuncStmt` lives in the root).
def FuncRef : Node<Atom> {
    let Fields = (ins
        Custom<FuncStmtRef>:$func,
        Custom<TypeRef>:$type
    );
}

/// Field / column access `base.field`; chains for multipart paths.
def FieldAtom : Node<Atom> {
    let Fields = (ins
        Child<Atom>:$base,
        Child<Ident>:$field,
        Custom<TypeRef>:$type
    );
}

def Constant : Variant<Atom> {}

def IntConst : Node<Constant> {
    let Fields = (ins Custom<Int64>:$value, Custom<TypeRef>:$type);
}

def FloatConst : Node<Constant> {
    let Fields = (ins Custom<Float64>:$value, Custom<TypeRef>:$type);
}

def BoolConst : Node<Constant> {
    let Fields = (ins Custom<Bool>:$value, Custom<TypeRef>:$type);
}

def StringConst : Node<Constant> {
    let Fields = (ins Custom<String>:$value, Custom<TypeRef>:$type);
}

/// A builtin operator applied to atomic arguments (`a + b`, `-x`).
def CallExpr : Node<Expr> {
    let Fields = (ins
        Custom<OpRef>:$op,
        Children<Atom>:$args,
        Custom<TypeRef>:$type
    );
}

/// A user-function call — the target a later beta-substitution pass inlines.
def FuncCallExpr : Node<Expr> {
    let Fields = (ins
        Child<Atom>:$callee,
        Children<Atom>:$args,
        Custom<TypeRef>:$type
    );
}

/// One `name: value` field initializer of a struct expression.
def StructFieldInit : Node<AnfNode> {
    let Fields = (ins Child<Ident>:$name, Child<Atom>:$value);
}

/// A struct construction `Employee { id: 1, name: "Bob" }` over atomic values.
def StructExpr : Node<Expr> {
    let Fields = (ins
        Child<Ident>:$name,
        Children<StructFieldInit>:$fields,
        Custom<TypeRef>:$type
    );
}

//===----------------------------------------------------------------------===//
// Blocks — defined here because the relational section below references
// `BlockStmt` (a column body).
//===----------------------------------------------------------------------===//

/// A statement sequence — a function body, or a per-row column body. Earlier
/// bindings scope over the statements that follow.
def BlockStmt : Node<Stmt> {
    let Fields = (ins Children<Stmt>:$stmts);
}

//===----------------------------------------------------------------------===//
// Relational operators — an `Expr` producing a relation. Inputs nest.
//===----------------------------------------------------------------------===//

def Rel : Variant<Expr> {}

/// The pipe source: `from <relation> as <alias>`.
def FromRel : Node<Rel> {
    let Fields = (ins
        Child<Ident>:$relation, 
        Child<Ident>:$alias,
        Custom<TypeRef>:$type
    );
}

/// One projected column. `body` is a per-row `BlockStmt` resolving to the
/// column's value at its tail; its `let`s are this column's temporaries (a
/// temp-free column is a block of just the tail). `alias` names the output and
/// is in scope for the items that follow it — lateral aliasing.
def SelectItem : Node<AnfNode> {
    let Fields = (ins
        Child<BlockStmt>:$body,
        Child<Ident>:$alias
    );
}

/// A `select` over an input relation. Column bodies read the input's row by
/// resolving the `from`'s alias; `items` are the output columns, lowered
/// left-to-right so each `alias` is in scope for the items that follow it.
def SelectRel : Node<Rel> {
    let Fields = (ins
        Child<Expr>:$input,
        Children<SelectItem>:$items,
        Custom<TypeRef>:$type
    );
}

//===----------------------------------------------------------------------===//
// Statements — bindings, control flow, and declarations.
//===----------------------------------------------------------------------===//

/// The binding itself — a named value definition that `VarAtom`s point back at.
/// `value` is null for a function parameter (an input, no defining expression).
/// `LetStmt` / `Param` wrap it to say which layer it lives in.
def Binding : Node<AnfNode> {
    let Fields = (ins
        Child<Ident>:$ident,
        Child<Expr>:$value,
        Custom<TypeRef>:$type
    );
}

/// A binding as a statement: `let <ident> = <value>`. Its scope (the "in
/// <rest>") is the statements that follow it in the enclosing `BlockStmt`.
def LetStmt : Node<Stmt> {
    let Fields = (ins Child<Binding>:$binding);
}

/// `return <value>` — a function body's control-flow tail.
def ReturnStmt : Node<Stmt> {
    let Fields = (ins Child<Atom>:$value);
}

/// A tail expression — the value an expression-shaped body yields (the query, a
/// column).
def ExprStmt : Node<Stmt> {
    let Fields = (ins 
        Child<Expr>:$value
    );
}

/// A binding as a function parameter — not a statement.
def Param : Node<AnfNode> {
    let Fields = (ins Child<Binding>:$binding);
}

/// A function definition; `body` is a `BlockStmt` ending in a `ReturnStmt`.
def FuncStmt : Node<Stmt> {
    let Fields = (ins
        Child<Ident>:$name,
        Children<Param>:$params,
        Child<BlockStmt>:$body,
        Custom<TypeRef>:$returnType
    );
}

/// One `name: type` field of a struct declaration.
def StructFieldDecl : Node<AnfNode> {
    let Fields = (ins 
        Child<Ident>:$name,
        Custom<TypeRef>:$type
    );
}

/// A nominal struct declaration.
def StructStmt : Node<Stmt> {
    let Fields = (ins Child<Ident>:$name, Children<StructFieldDecl>:$fields);
}

/// A table registering a relation; `type` is its `Relation[row]`.
def TableStmt : Node<Stmt> {
    let Fields = (ins Child<Ident>:$name, Custom<TypeRef>:$type);
}

/// Top of an ANF tree: the program, a flat sequence of statements.
def Root : Node<AnfNode> {
    let Fields = (ins Children<Stmt>:$stmts);
}
