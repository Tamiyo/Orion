class Token<string name> {
    string Name = name;
}

class SymbolToken<
    string symbol,
    string name
> : Token<name> {
    string Symbol = symbol;
}

class KeywordToken<
    string name
> : Token<name> {
}

class LiteralToken<
    string name
> : Token<name> {
}

class BasicToken<
    string name
> : Token<name> {
}
