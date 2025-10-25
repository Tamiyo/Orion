class AstNode {
    string SyntaxKind;
}

class AstMethod {
    string IteratorType;
    string BuilderType;
    string ReturnType;
}

def NthAstMethod : AstMethod {
    int Nth;
}
