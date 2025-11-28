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
    string symbol,
    string name
> : Token<name> {
    string Symbol = symbol;
}

class LiteralToken<
    string name
> : Token<name> {
}

class TriviaToken<
    string name
> : Token<name> {
}

class BasicToken<
    string name
> : Token<name> {
}
