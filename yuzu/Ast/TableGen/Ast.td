class AstMethod<string name> {
    string Name = name;
}

class ChildAstMethod<
    string name,
    int n,
    string subclass
> : AstMethod<name> {
    int N = n;
    string AstNodeSubclass = subclass;
}

class ChildrenAstMethod<
    string name,
    string subclass
> : AstMethod<name> {
    string AstNodeSubclass = subclass;
}

class TokenAstMethod<
    string name,
    int n
> : AstMethod<name> {
    int N = n;
}

class AstNode {
    string SyntaxKind;
    list<AstMethod> Methods = [];
}
