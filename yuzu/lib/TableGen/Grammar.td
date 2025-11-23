include "GrammarBase.td"

def YuzuGrammar : Grammar {
    // The grammar does not (yet) generate rules in the lexer, the symbols are for show for now.
    let Tokens = [
        // Symbols
        SymbolToken<"=", "Equals">,
        SymbolToken<"+", "Plus">,
        SymbolToken<"-", "Minus">,
        SymbolToken<"*", "Star">,
        SymbolToken<"/", "Slash">,

        // Literals
        LiteralToken<"Number">,

        // Whitespace
        WhitespaceToken<"Space">,
        WhitespaceToken<"Newline">,
        WhitespaceToken<"Comment">,

        // Keyword
        KeywordToken<"let", "Let">,

        // Basic Tokens
        BasicToken<"Ident">
    ];

    let Nodes = [
        // Expressions
        AbstractAstNode<"Expr">,
        ConcreteAstNode<
            "BinaryExpr", 
            "Expr", 
            [
                ChildAstMethod<"Lhs", "Expr">,
                TokenAstMethod<"Op">,
                ChildAstMethod<"Rhs", "Expr", 1>
            ]
        >,
        ConcreteAstNode<
            "ParenExpr", 
            "Expr",
            [
                ChildAstMethod<"Expr", "Expr">
            ]
        >,
        ConcreteAstNode<
            "LiteralExpr", 
            "Expr", 
            [
                TokenAstMethod<"Value", "Number">
            ]
        >
    ];
}
