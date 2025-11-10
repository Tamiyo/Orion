include "GrammarBase.td"

def YuzuGrammar : Grammar {
    let Tokens = [
        // Symbols
        SymbolToken<"=", "Equals">,
        SymbolToken<"+", "Plus">,
        SymbolToken<"-", "Minus">,
        SymbolToken<"*", "Star">,
        SymbolToken<"/", "Slash">,

        // Literals
        LiteralToken<"Number">,
    ];

    let Nodes = [
        AbstractAstNode<"Expr">, // EXPRESSIONS11
        ConcreteAstNode<
            "BinaryExpr", 
            "Expr", [
                ChildAstMethod<"Lhs", "Expr">,
                TokenAstMethod<"Op">,
                ChildAstMethod<"Rhs", "Expr", 1>
            ]
        >,
        ConcreteAstNode<
            "ParenExpr", 
            "Expr", [
                ChildAstMethod<"Expr", "Expr">
            ]
        >,
        ConcreteAstNode<
            "LiteralExpr", 
            "Expr", [
                TokenAstMethod<"Value", "Number">
            ]
        >
    ];
}
