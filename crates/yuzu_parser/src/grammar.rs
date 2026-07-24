use yuzu_lexer::token_kind::TokenKind;
use yuzu_syntax::SyntaxKind;

use crate::parser::{Parser, marker::CompletedMarker};

mod expr;
mod rel;
mod stmt;
mod ty;

pub(crate) fn parse_root(p: &mut Parser) -> CompletedMarker {
    let m = p.start();
    while !p.at_end() {
        stmt::parse_stmt(p);
    }
    p.complete(m, SyntaxKind::Root)
}

pub(crate) fn parse_ident(p: &mut Parser) -> CompletedMarker {
    let m = p.start();
    p.expect(TokenKind::Identifier);
    p.complete(m, SyntaxKind::Ident)
}

#[cfg(test)]
mod test_support {
    use expect_test::Expect;
    use yuzu_diagnostics::diagnostics::engine::DiagnosticsEngine;
    use yuzu_diagnostics::source_map::SourceMap;
    use yuzu_lexer::lexer::{Lexer, Token};
    use yuzu_syntax::SyntaxNode;

    use crate::parser::Parser;
    use crate::token_sink::TokenSink;
    use crate::token_source::TokenSource;

    pub(crate) fn check<R>(input: &str, parse: impl FnOnce(&mut Parser) -> R, expected: Expect) {
        let tokens: Vec<Token> = Lexer::new(input).collect();
        let mut sources = SourceMap::new();
        let source_id = sources.add("test".to_string(), input.to_string());

        let mut parser = Parser::new(TokenSource::new(&tokens), source_id);
        parse(&mut parser);
        let events = parser.finish();

        let mut diagnostics = DiagnosticsEngine::new();
        let result = TokenSink::new(&tokens, events, &mut diagnostics).finish();
        let tree = SyntaxNode::new_root(result.green);

        expected.assert_eq(&format!("{tree:#?}"));
    }
}

#[cfg(test)]
mod tests {
    use expect_test::{Expect, expect};

    use super::{parse_ident, parse_root};
    use crate::grammar::test_support;

    #[test]
    fn parse_ident_directly() {
        test_support::check(
            "foo",
            parse_ident,
            expect![[r#"
            Ident@0..3
              Identifier@0..3 "foo"
        "#]],
        );
    }

    #[test]
    fn parse_root_directly() {
        test_support::check(
            "",
            parse_root,
            expect![[r#"
            Root@0..0
        "#]],
        );
    }
}
