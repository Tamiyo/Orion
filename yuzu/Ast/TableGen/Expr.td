include "BuiltIn.td"

class Expr : AstNode {
}

def BinaryExpr : Expr {
    let SyntaxKind = "InfixExpr";
}

def ParenExpr : Expr {
    let SyntaxKind = "ParenExpr";
}

def LiteralExpr : Expr {
    let SyntaxKind = "LiteralExpr";
}
