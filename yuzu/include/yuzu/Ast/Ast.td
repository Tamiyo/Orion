include "yuzu/DSL/TreeBase.td"

// Base
def AstNode : Base { 
    let Namespace = "yuzu::ast";
} 

// Stmts
def Stmt : Variant<AstNode> {}

// Expressions
def Expr : Variant<AstNode> {}


def BinaryExpr : Node<Expr> {
    let Fields = (ins 
        Child<Expr>:$lhs, 
        Child<Expr>:$rhs
    );
}

def ParenExpr : Node<Expr> {
    let Fields = (ins 
        Child<Expr>:$expr
    );
}

def LiteralExpr : Node<Expr> {
}
