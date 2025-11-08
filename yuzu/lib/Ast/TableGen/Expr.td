include "Ast.td"

class Expr : AstNode {
}

def BinaryExpr : Expr {
    let SyntaxKind = "InfixExpr";
    let Methods = [
        ChildAstMethod<"Lhs", "Expr">,
        TokenAstMethod<"Op">,
        ChildAstMethod<"Rhs", "Expr", 1>,
    ];
}

def ParenExpr : Expr {
    let SyntaxKind = "ParenExpr";
    let Methods = [
        ChildAstMethod<"Expr", "Expr">,
    ];
}

def LiteralExpr : Expr {
    let SyntaxKind = "LiteralExpr";
     let Methods = [
        TokenAstMethod<"Value", "Number">,
    ];
}
