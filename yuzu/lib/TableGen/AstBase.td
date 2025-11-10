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

class AstNode<
    string name, 
    string parent,
    bit isAbstract,
    list<AstMethod> methods
> {
    string Name = name;
    string Parent = parent;
    bit IsAbstract = isAbstract;
    list<AstMethod> Methods = methods;
}

class AbstractAstNode<
    string name, 
    string parent = "",
    list<AstMethod> methods = []
> : AstNode<name, parent, 1, methods> {
}

class ConcreteAstNode<
    string name, 
    string parent = "",
    list<AstMethod> methods = []
> : AstNode<name, parent, 0, methods> {
}
