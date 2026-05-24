//===----------------------------------------------------------------------===//
// Lexical token schema.
//===----------------------------------------------------------------------===//
//
// Tokens come in two shapes — exact-string matches (`Token`) and
// regex-pattern matches (`Regex`) — both deriving from `Metadata`, which
// carries the category flags the codegen uses to bucket tokens. Each token
// is a top-level `def`; the def's name is the kind name (e.g.
// `def Plus : Token<["+"]>;` produces `TokenKind::Plus`).
//
//===----------------------------------------------------------------------===//

/// Lex grammar config. Carries the C++ namespace the emitted `TokenKind`
/// enum (and its predicates) live in. Exactly one `LexGrammar` def is
/// expected per .td; the generator reads `Namespace` off it.
class LexGrammar {
    string Namespace = "";
}

/// Common base for every lex token. The `Is*` flags categorize the token;
/// downstream codegen reads them to emit per-category predicates.
///
/// `Display` is the prose form rendered inside diagnostics ("expected
/// `<Display>`, found ..."). If empty, the codegen derives a default:
/// a single-value `Token` becomes `` `<value>` ``; everything else falls
/// back to the def name. Override on regex/multi-value defs (Ident,
/// IntegerLiteral, BooleanLiteral, ...) to give them a category name.
class Metadata {
    bit IsSymbol = 0;
    bit IsPunctuation = 0;
    bit IsKeyword = 0;
    bit IsLiteral = 0;
    bit IsTrivia = 0;
    string Display = "";
}

/// A token matched by any of the exact strings in `values`. `callback` is
/// optional C++ code run on match; `priority` resolves matching ties when
/// more than one token would accept the same input.
class Token<
    list<string> values = [],
    code callback = "",
    int priority = 0
> : Metadata {
    list<string> Values = values;
    code Callback = callback;
    int Priority = priority;
}

/// A token matched by any of the regex `patterns`. Same `callback`/`priority`
/// semantics as `Token`.
class Regex<
    list<string> patterns = [],
    code callback = "",
    int priority = 0
> : Metadata {
    list<string> Patterns = patterns;
    code Callback = callback;
    int Priority = priority;
}

