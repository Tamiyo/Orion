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

def Bool : Native<"bool"> {}
def Int64 : Native<"int64_t"> {}
def Float64 : Native<"double"> {}
def String : Native<"std::u32string"> {}

/// A binary operator.
def BinOp : Enum<"uint8_t"> {
    let Cases = [
        EnumCase<"Add">,
        EnumCase<"Sub">,
        EnumCase<"Mul">,
        EnumCase<"Div">
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
// Expressions
//===----------------------------------------------------------------------===//

/// Top-level variant for anything that produces a value.
def Expr : Variant<AstNode> {}

/// Binary infix expression like `a + b`. The operator token isn't carried
/// in the AST — recover it by inspecting the underlying syntax tree.
def BinaryExpr : Node<Expr> {
    let Fields = (ins
        Child<Expr>:$lhs,
        Custom<BinOp>:$op,
        Child<Expr>:$rhs
    );
}

/// A literal expression. Currently always a number; the literal value lives
/// on the syntax token, not in the AST.
def Literal : Variant<Expr> {}

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
def Stmt : Variant<AstNode> {}

/// Statement wrapping a single expression: `42`, `a + b`, etc.
def ExprStmt : Node<Stmt> {
    let Fields = (ins
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
