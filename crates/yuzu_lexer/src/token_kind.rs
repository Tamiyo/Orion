use logos::Logos;

#[derive(Logos, Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
#[repr(u16)]
#[logos(utf8 = true)]
pub enum TokenKind {
    #[token("+", priority = 1)]
    Plus,

    #[token("-", priority = 1)]
    Minus,

    #[token("*", priority = 1)]
    Star,

    #[token("**", priority = 1)]
    StarStar,

    #[token("/", priority = 1)]
    Slash,

    #[token("=", priority = 1)]
    Eq,

    #[token("==", priority = 1)]
    EqEq,

    #[token("!=", priority = 1)]
    Neq,

    #[token("<", priority = 1)]
    Lt,

    #[token("<=", priority = 1)]
    Lte,

    #[token(">", priority = 1)]
    Gt,

    #[token(">=", priority = 1)]
    Gte,

    #[token("<<", priority = 1)]
    Shl,

    #[token(">>", priority = 1)]
    Shr,

    #[token("->", priority = 1)]
    Arrow,

    #[token("|>", priority = 1)]
    Pipe,

    #[token(".", priority = 1)]
    Dot,

    #[token("(", priority = 1)]
    LeftParen,

    #[token(")", priority = 1)]
    RightParen,

    #[token("{", priority = 1)]
    LeftCurly,

    #[token("}", priority = 1)]
    RightCurly,

    #[token("[", priority = 1)]
    LeftSquare,

    #[token("]", priority = 1)]
    RightSquare,

    #[token(",", priority = 1)]
    Comma,

    #[token(":", priority = 1)]
    Colon,

    #[token("and", priority = 1)]
    AndKw,

    #[token("as", priority = 1)]
    AsKw,

    #[token("distinct", priority = 1)]
    DistinctKw,

    #[token("drop", priority = 1)]
    DropKw,

    #[token("extend", priority = 1)]
    ExtendKw,

    #[token("fn", priority = 1)]
    FnKw,

    #[token("for", priority = 1)]
    ForKw,

    #[token("from", priority = 1)]
    FromKw,

    #[token("impl", priority = 1)]
    ImplKw,

    #[token("in", priority = 1)]
    InKw,

    #[token("let", priority = 1)]
    LetKw,

    #[token("mut", priority = 1)]
    MutKw,

    #[token("not", priority = 1)]
    NotKw,

    #[token("or", priority = 1)]
    OrKw,

    #[token("rename", priority = 1)]
    RenameKw,

    #[token("return", priority = 1)]
    ReturnKw,

    #[token("select", priority = 1)]
    SelectKw,

    #[token("struct", priority = 1)]
    StructKw,

    #[token("table", priority = 1)]
    TableKw,

    #[token("trait", priority = 1)]
    TraitKw,

    #[token("where", priority = 1)]
    WhereKw,

    #[regex("[a-zA-Z_][a-zA-Z0-9_]*", priority = 0)]
    Identifier,

    #[token("true", priority = 1)]
    #[token("false", priority = 1)]
    BoolLit,

    #[regex("[0-9][0-9_]*", priority = 1)]
    IntLit,

    #[regex(r#"([0-9][0-9_]*\.[0-9_]*|\.[0-9][0-9_]*)([eE][+-]?[0-9][0-9_]*)?|[0-9][0-9_]*[eE][+-]?[0-9][0-9_]*"#, priority = 1)]
    FloatLit,

    #[regex(r#"0[xX][0-9a-fA-F][0-9a-fA-F_]*"#, priority = 1)]
    HexLit,

    #[regex(r#"0[bB][01][01_]*"#, priority = 1)]
    BinaryLit,

    #[regex(r#""([^"\\]|\\.)*""#, priority = 1)]
    StringLit,

    #[regex(r#"r"[^"]*""#, priority = 1)]
    RawStringLit,

    #[regex(r#"//.*"#, priority = 1, allow_greedy = true)]
    Comment,

    #[regex(r#"\r?\n"#, priority = 1)]
    Newline,

    #[regex(r#" "#, priority = 1)]
    Space,

    // Not a lexable token. Created when the lexer does not recognize a token.
    Error,
}

impl TokenKind {
    pub fn is_trivia(self) -> bool {
        matches!(
            self,
            TokenKind::Comment | TokenKind::Space | TokenKind::Newline
        )
    }
}

impl std::fmt::Display for TokenKind {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let text = match self {
            TokenKind::Plus => "+",
            TokenKind::Minus => "-",
            TokenKind::Star => "*",
            TokenKind::StarStar => "**",
            TokenKind::Slash => "/",
            TokenKind::Eq => "=",
            TokenKind::EqEq => "==",
            TokenKind::Neq => "!=",
            TokenKind::Lt => "<",
            TokenKind::Lte => "<=",
            TokenKind::Gt => ">",
            TokenKind::Gte => ">=",
            TokenKind::Shl => "<<",
            TokenKind::Shr => ">>",
            TokenKind::Arrow => "->",
            TokenKind::Pipe => "|>",
            TokenKind::Dot => ".",
            TokenKind::LeftParen => "(",
            TokenKind::RightParen => ")",
            TokenKind::LeftCurly => "{",
            TokenKind::RightCurly => "}",
            TokenKind::LeftSquare => "[",
            TokenKind::RightSquare => "]",
            TokenKind::Comma => ",",
            TokenKind::Colon => ":",
            TokenKind::AndKw => "and",
            TokenKind::AsKw => "as",
            TokenKind::DistinctKw => "distinct",
            TokenKind::DropKw => "drop",
            TokenKind::ExtendKw => "extend",
            TokenKind::FnKw => "fn",
            TokenKind::ForKw => "for",
            TokenKind::FromKw => "from",
            TokenKind::ImplKw => "impl",
            TokenKind::InKw => "in",
            TokenKind::LetKw => "let",
            TokenKind::MutKw => "mut",
            TokenKind::NotKw => "not",
            TokenKind::OrKw => "or",
            TokenKind::RenameKw => "rename",
            TokenKind::ReturnKw => "return",
            TokenKind::SelectKw => "select",
            TokenKind::StructKw => "struct",
            TokenKind::TableKw => "table",
            TokenKind::TraitKw => "trait",
            TokenKind::WhereKw => "where",
            TokenKind::Identifier => "identifier",
            TokenKind::BoolLit => "boolean literal",
            TokenKind::IntLit => "integer literal",
            TokenKind::FloatLit => "float literal",
            TokenKind::HexLit => "hex literal",
            TokenKind::BinaryLit => "binary literal",
            TokenKind::StringLit => "string literal",
            TokenKind::RawStringLit => "raw string literal",
            TokenKind::Comment => "comment",
            TokenKind::Newline => "newline",
            TokenKind::Space => "whitespace",
            TokenKind::Error => "invalid token",
        };
        f.write_str(text)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn display_renders_every_variant() {
        let cases = [
            (TokenKind::Plus, "+"),
            (TokenKind::Minus, "-"),
            (TokenKind::Star, "*"),
            (TokenKind::StarStar, "**"),
            (TokenKind::Slash, "/"),
            (TokenKind::Eq, "="),
            (TokenKind::EqEq, "=="),
            (TokenKind::Neq, "!="),
            (TokenKind::Lt, "<"),
            (TokenKind::Lte, "<="),
            (TokenKind::Gt, ">"),
            (TokenKind::Gte, ">="),
            (TokenKind::Shl, "<<"),
            (TokenKind::Shr, ">>"),
            (TokenKind::Arrow, "->"),
            (TokenKind::Pipe, "|>"),
            (TokenKind::Dot, "."),
            (TokenKind::LeftParen, "("),
            (TokenKind::RightParen, ")"),
            (TokenKind::LeftCurly, "{"),
            (TokenKind::RightCurly, "}"),
            (TokenKind::LeftSquare, "["),
            (TokenKind::RightSquare, "]"),
            (TokenKind::Comma, ","),
            (TokenKind::Colon, ":"),
            (TokenKind::AndKw, "and"),
            (TokenKind::AsKw, "as"),
            (TokenKind::DistinctKw, "distinct"),
            (TokenKind::DropKw, "drop"),
            (TokenKind::ExtendKw, "extend"),
            (TokenKind::FnKw, "fn"),
            (TokenKind::ForKw, "for"),
            (TokenKind::FromKw, "from"),
            (TokenKind::ImplKw, "impl"),
            (TokenKind::InKw, "in"),
            (TokenKind::LetKw, "let"),
            (TokenKind::MutKw, "mut"),
            (TokenKind::NotKw, "not"),
            (TokenKind::OrKw, "or"),
            (TokenKind::RenameKw, "rename"),
            (TokenKind::ReturnKw, "return"),
            (TokenKind::SelectKw, "select"),
            (TokenKind::StructKw, "struct"),
            (TokenKind::TableKw, "table"),
            (TokenKind::TraitKw, "trait"),
            (TokenKind::WhereKw, "where"),
            (TokenKind::Identifier, "identifier"),
            (TokenKind::BoolLit, "boolean literal"),
            (TokenKind::IntLit, "integer literal"),
            (TokenKind::FloatLit, "float literal"),
            (TokenKind::HexLit, "hex literal"),
            (TokenKind::BinaryLit, "binary literal"),
            (TokenKind::StringLit, "string literal"),
            (TokenKind::RawStringLit, "raw string literal"),
            (TokenKind::Comment, "comment"),
            (TokenKind::Newline, "newline"),
            (TokenKind::Space, "whitespace"),
            (TokenKind::Error, "invalid token"),
        ];

        for (kind, expected) in cases {
            assert_eq!(kind.to_string(), expected, "{kind:?}");
        }
    }
}
