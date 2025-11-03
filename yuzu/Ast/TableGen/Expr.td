include "Ast.td"

class Expr : AstNode {
}

def BinaryExpr : Expr {
    let SyntaxKind = "InfixExpr";
    let Methods = [
        ChildAstMethod<"Lhs", 0, "Expr">,
        ChildAstMethod<"Rhs", 1, "Expr">,
    ];
}

def ParenExpr : Expr {
    let SyntaxKind = "ParenExpr";
    let Methods = [
        ChildAstMethod<"Expr", 0, "Expr">,
    ];
}

def LiteralExpr : Expr {
    let SyntaxKind = "LiteralExpr";
}
