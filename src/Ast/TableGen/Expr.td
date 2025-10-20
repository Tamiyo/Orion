class AstNode {
    bit IsAbstract = 1;

    list<string> ProtectedMembers = [
        "syntax::SyntaxNode Node_"
    ];

    list<string> BaseIncludes = [
        "Syntax/Syntax.h",
        "memory",
        "utility"
    ];
}

class Expr : AstNode {
    let IsAbstract = 1;
}

def BinaryExpr : Expr {
    let IsAbstract = 0;
}
