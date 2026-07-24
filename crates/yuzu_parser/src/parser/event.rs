use yuzu_syntax::SyntaxKind;

use crate::parser::error::ParseError;

pub(crate) enum Event {
    Start {
        kind: SyntaxKind,
        forward_parent: Option<usize>,
    },
    Token,
    Finish,
    Error {
        error: ParseError,
    },
    Placeholder,
}
