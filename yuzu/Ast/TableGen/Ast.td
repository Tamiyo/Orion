class AstMethod<string name> {
    string Name = name;
}

class ChildAstMethod<
    string name,
    string subclass,
    int n = 0
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
    string kind = "",
    int n = 0
> : AstMethod<name> {
    int N = n;
    string Kind = kind;
}

class AstNode {
    string SyntaxKind;
    list<AstMethod> Methods = [];
}
