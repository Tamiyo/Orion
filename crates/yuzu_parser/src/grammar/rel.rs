use yuzu_lexer::token_kind::TokenKind;
use yuzu_syntax::SyntaxKind;

use crate::grammar::expr::parse_expr;
use crate::grammar::parse_ident;
use crate::parser::{Parser, marker::CompletedMarker};

pub(crate) fn parse_query(p: &mut Parser) -> CompletedMarker {
    let mut query = parse_from_expr(p);

    while p.at(TokenKind::Pipe) {
        let marker = p.precede(query);
        p.bump();

        query = if p.at(TokenKind::WhereKw) {
            parse_where_clause(p);
            p.complete(marker, SyntaxKind::WhereExpr)
        } else if p.at(TokenKind::DistinctKw) {
            parse_distinct_clause(p);
            p.complete(marker, SyntaxKind::DistinctExpr)
        } else if p.at(TokenKind::DropKw) {
            parse_drop_clause(p);
            p.complete(marker, SyntaxKind::DropExpr)
        } else if p.at(TokenKind::RenameKw) {
            parse_rename_clause(p);
            p.complete(marker, SyntaxKind::RenameExpr)
        } else if p.at(TokenKind::ExtendKw) {
            parse_extend_clause(p);
            p.complete(marker, SyntaxKind::ExtendExpr)
        } else if at_join_clause(p) {
            parse_join_clause(p);
            p.complete(marker, SyntaxKind::JoinExpr)
        } else {
            parse_select_clause(p);
            p.complete(marker, SyntaxKind::SelectExpr)
        };
    }

    query
}

fn parse_from_expr(p: &mut Parser) -> CompletedMarker {
    let m = p.start();
    p.expect(TokenKind::FromKw);
    parse_ident(p);

    if p.at(TokenKind::AsKw) {
        p.bump();
        parse_ident(p);
    } else if p.at(TokenKind::Identifier) {
        parse_ident(p);
    }

    p.complete(m, SyntaxKind::FromExpr)
}

fn parse_select_item(p: &mut Parser) -> CompletedMarker {
    let m = p.start();
    parse_expr(p);

    if p.at(TokenKind::AsKw) {
        p.bump();
        parse_ident(p);
    }

    p.complete(m, SyntaxKind::SelectItem)
}

fn parse_select_clause(p: &mut Parser) {
    p.expect(TokenKind::SelectKw);
    parse_select_item(p);
    while p.at(TokenKind::Comma) {
        p.bump();
        parse_select_item(p);
    }
}

fn parse_where_clause(p: &mut Parser) {
    p.expect(TokenKind::WhereKw);
    parse_expr(p);
}

fn parse_distinct_clause(p: &mut Parser) {
    p.expect(TokenKind::DistinctKw);
}

fn parse_drop_clause(p: &mut Parser) {
    p.expect(TokenKind::DropKw);
    parse_ident(p);
    while p.at(TokenKind::Comma) {
        p.bump();
        parse_ident(p);
    }
}

fn parse_rename_item(p: &mut Parser) {
    let m = p.start();
    parse_ident(p);
    if p.at(TokenKind::Dot) {
        p.bump();
        parse_ident(p);
    }
    p.expect(TokenKind::AsKw);
    parse_ident(p);
    p.complete(m, SyntaxKind::RenameItem);
}

fn parse_rename_clause(p: &mut Parser) {
    p.expect(TokenKind::RenameKw);
    parse_rename_item(p);
    while p.at(TokenKind::Comma) {
        p.bump();
        parse_rename_item(p);
    }
}

fn parse_extend_clause(p: &mut Parser) {
    p.expect(TokenKind::ExtendKw);
    parse_select_item(p);
    while p.at(TokenKind::Comma) {
        p.bump();
        parse_select_item(p);
    }
}

const JOIN_TYPES: [TokenKind; 4] = [
    TokenKind::InnerKw,
    TokenKind::LeftKw,
    TokenKind::RightKw,
    TokenKind::FullKw,
];

fn at_join_clause(p: &mut Parser) -> bool {
    p.at(TokenKind::JoinKw) || at_join_type(p)
}

fn at_join_type(p: &mut Parser) -> bool {
    JOIN_TYPES.iter().any(|&kind| p.at(kind))
}

fn parse_join_clause(p: &mut Parser) {
    if at_join_type(p) {
        p.bump();
    }
    p.expect(TokenKind::JoinKw);
    parse_ident(p);

    if p.at(TokenKind::AsKw) {
        p.bump();
        parse_ident(p);
    } else if p.at(TokenKind::Identifier) {
        parse_ident(p);
    }

    if p.at(TokenKind::UsingKw) {
        parse_join_using(p);
    } else {
        parse_join_on(p);
    }
}

fn parse_join_on(p: &mut Parser) {
    let m = p.start();
    p.expect(TokenKind::OnKw);
    parse_expr(p);
    p.complete(m, SyntaxKind::JoinOn);
}

fn parse_join_using(p: &mut Parser) {
    let m = p.start();
    p.expect(TokenKind::UsingKw);
    p.expect(TokenKind::LeftParen);
    // An empty list is left to lowering to report; parsing an identifier here
    // would consume the `)` into an error node and name a column after it.
    if !p.at(TokenKind::RightParen) {
        parse_ident(p);
        while p.at(TokenKind::Comma) {
            p.bump();
            parse_ident(p);
        }
    }
    p.expect(TokenKind::RightParen);
    p.complete(m, SyntaxKind::JoinUsing);
}

#[cfg(test)]
mod tests {
    use expect_test::expect;

    use super::*;
    use crate::grammar::test_support;

    #[test]
    fn parse_from_expr_directly() {
        test_support::check(
            "from employees",
            parse_from_expr,
            expect![[r#"
            FromExpr@0..14
              FromKw@0..4 "from"
              Space@4..5 " "
              Ident@5..14
                Identifier@5..14 "employees"
        "#]],
        );
    }

    #[test]
    fn parse_from_expr_with_alias() {
        test_support::check(
            "from employees as e",
            parse_from_expr,
            expect![[r#"
                FromExpr@0..19
                  FromKw@0..4 "from"
                  Space@4..5 " "
                  Ident@5..14
                    Identifier@5..14 "employees"
                  Space@14..15 " "
                  AsKw@15..17 "as"
                  Space@17..18 " "
                  Ident@18..19
                    Identifier@18..19 "e"
            "#]],
        );
    }

    #[test]
    fn parse_select_item_directly() {
        test_support::check(
            "name as n",
            parse_select_item,
            expect![[r#"
                SelectItem@0..9
                  IdentExpr@0..4
                    Ident@0..4
                      Identifier@0..4 "name"
                  Space@4..5 " "
                  AsKw@5..7 "as"
                  Space@7..8 " "
                  Ident@8..9
                    Identifier@8..9 "n"
            "#]],
        );
    }

    #[test]
    fn parse_rename_item_directly() {
        test_support::check(
            "old as new",
            parse_rename_item,
            expect![[r#"
                RenameItem@0..10
                  Ident@0..3
                    Identifier@0..3 "old"
                  Space@3..4 " "
                  AsKw@4..6 "as"
                  Space@6..7 " "
                  Ident@7..10
                    Identifier@7..10 "new"
            "#]],
        );
    }

    #[test]
    fn parse_query_select_clause() {
        test_support::check(
            "from t |> select a, b",
            parse_query,
            expect![[r#"
                SelectExpr@0..21
                  FromExpr@0..6
                    FromKw@0..4 "from"
                    Space@4..5 " "
                    Ident@5..6
                      Identifier@5..6 "t"
                  Space@6..7 " "
                  Pipe@7..9 "|>"
                  Space@9..10 " "
                  SelectKw@10..16 "select"
                  Space@16..17 " "
                  SelectItem@17..18
                    IdentExpr@17..18
                      Ident@17..18
                        Identifier@17..18 "a"
                  Comma@18..19 ","
                  Space@19..20 " "
                  SelectItem@20..21
                    IdentExpr@20..21
                      Ident@20..21
                        Identifier@20..21 "b"
            "#]],
        );
    }

    #[test]
    fn parse_query_where_clause() {
        test_support::check(
            "from t |> where active",
            parse_query,
            expect![[r#"
                WhereExpr@0..22
                  FromExpr@0..6
                    FromKw@0..4 "from"
                    Space@4..5 " "
                    Ident@5..6
                      Identifier@5..6 "t"
                  Space@6..7 " "
                  Pipe@7..9 "|>"
                  Space@9..10 " "
                  WhereKw@10..15 "where"
                  Space@15..16 " "
                  IdentExpr@16..22
                    Ident@16..22
                      Identifier@16..22 "active"
            "#]],
        );
    }

    #[test]
    fn parse_query_distinct_clause() {
        test_support::check(
            "from t |> distinct",
            parse_query,
            expect![[r#"
                DistinctExpr@0..18
                  FromExpr@0..6
                    FromKw@0..4 "from"
                    Space@4..5 " "
                    Ident@5..6
                      Identifier@5..6 "t"
                  Space@6..7 " "
                  Pipe@7..9 "|>"
                  Space@9..10 " "
                  DistinctKw@10..18 "distinct"
            "#]],
        );
    }

    #[test]
    fn parse_query_drop_clause() {
        test_support::check(
            "from t |> drop a, b",
            parse_query,
            expect![[r#"
                DropExpr@0..19
                  FromExpr@0..6
                    FromKw@0..4 "from"
                    Space@4..5 " "
                    Ident@5..6
                      Identifier@5..6 "t"
                  Space@6..7 " "
                  Pipe@7..9 "|>"
                  Space@9..10 " "
                  DropKw@10..14 "drop"
                  Space@14..15 " "
                  Ident@15..16
                    Identifier@15..16 "a"
                  Comma@16..17 ","
                  Space@17..18 " "
                  Ident@18..19
                    Identifier@18..19 "b"
            "#]],
        );
    }

    #[test]
    fn parse_query_rename_clause() {
        test_support::check(
            "from t |> rename a as b",
            parse_query,
            expect![[r#"
                RenameExpr@0..23
                  FromExpr@0..6
                    FromKw@0..4 "from"
                    Space@4..5 " "
                    Ident@5..6
                      Identifier@5..6 "t"
                  Space@6..7 " "
                  Pipe@7..9 "|>"
                  Space@9..10 " "
                  RenameKw@10..16 "rename"
                  Space@16..17 " "
                  RenameItem@17..23
                    Ident@17..18
                      Identifier@17..18 "a"
                    Space@18..19 " "
                    AsKw@19..21 "as"
                    Space@21..22 " "
                    Ident@22..23
                      Identifier@22..23 "b"
            "#]],
        );
    }

    #[test]
    fn parse_query_extend_clause() {
        test_support::check(
            "from t |> extend a",
            parse_query,
            expect![[r#"
                ExtendExpr@0..18
                  FromExpr@0..6
                    FromKw@0..4 "from"
                    Space@4..5 " "
                    Ident@5..6
                      Identifier@5..6 "t"
                  Space@6..7 " "
                  Pipe@7..9 "|>"
                  Space@9..10 " "
                  ExtendKw@10..16 "extend"
                  Space@16..17 " "
                  SelectItem@17..18
                    IdentExpr@17..18
                      Ident@17..18
                        Identifier@17..18 "a"
            "#]],
        );
    }

    #[test]
    fn parse_rename_item_qualified() {
        test_support::check(
            "e.id as eid",
            parse_rename_item,
            expect![[r#"
            RenameItem@0..11
              Ident@0..1
                Identifier@0..1 "e"
              Dot@1..2 "."
              Ident@2..4
                Identifier@2..4 "id"
              Space@4..5 " "
              AsKw@5..7 "as"
              Space@7..8 " "
              Ident@8..11
                Identifier@8..11 "eid"
        "#]],
        );
    }

    #[test]
    fn parse_query_join_on_clause() {
        test_support::check(
            "from t |> join u as d on a == d.b",
            parse_query,
            expect![[r#"
                JoinExpr@0..33
                  FromExpr@0..6
                    FromKw@0..4 "from"
                    Space@4..5 " "
                    Ident@5..6
                      Identifier@5..6 "t"
                  Space@6..7 " "
                  Pipe@7..9 "|>"
                  Space@9..10 " "
                  JoinKw@10..14 "join"
                  Space@14..15 " "
                  Ident@15..16
                    Identifier@15..16 "u"
                  Space@16..17 " "
                  AsKw@17..19 "as"
                  Space@19..20 " "
                  Ident@20..21
                    Identifier@20..21 "d"
                  Space@21..22 " "
                  JoinOn@22..33
                    OnKw@22..24 "on"
                    Space@24..25 " "
                    BinaryExpr@25..33
                      IdentExpr@25..26
                        Ident@25..26
                          Identifier@25..26 "a"
                      Space@26..27 " "
                      EqEq@27..29 "=="
                      Space@29..30 " "
                      FieldAccessExpr@30..33
                        IdentExpr@30..31
                          Ident@30..31
                            Identifier@30..31 "d"
                        Dot@31..32 "."
                        Ident@32..33
                          Identifier@32..33 "b"
            "#]],
        );
    }

    #[test]
    fn parse_query_join_using_clause() {
        test_support::check(
            "from t |> left join u using (a, b)",
            parse_query,
            expect![[r#"
                JoinExpr@0..34
                  FromExpr@0..6
                    FromKw@0..4 "from"
                    Space@4..5 " "
                    Ident@5..6
                      Identifier@5..6 "t"
                  Space@6..7 " "
                  Pipe@7..9 "|>"
                  Space@9..10 " "
                  LeftKw@10..14 "left"
                  Space@14..15 " "
                  JoinKw@15..19 "join"
                  Space@19..20 " "
                  Ident@20..21
                    Identifier@20..21 "u"
                  Space@21..22 " "
                  JoinUsing@22..34
                    UsingKw@22..27 "using"
                    Space@27..28 " "
                    LeftParen@28..29 "("
                    Ident@29..30
                      Identifier@29..30 "a"
                    Comma@30..31 ","
                    Space@31..32 " "
                    Ident@32..33
                      Identifier@32..33 "b"
                    RightParen@33..34 ")"
            "#]],
        );
    }

    #[test]
    fn parse_query_join_bare_alias() {
        test_support::check(
            "from t |> full join u d on a == d.b",
            parse_query,
            expect![[r#"
                JoinExpr@0..35
                  FromExpr@0..6
                    FromKw@0..4 "from"
                    Space@4..5 " "
                    Ident@5..6
                      Identifier@5..6 "t"
                  Space@6..7 " "
                  Pipe@7..9 "|>"
                  Space@9..10 " "
                  FullKw@10..14 "full"
                  Space@14..15 " "
                  JoinKw@15..19 "join"
                  Space@19..20 " "
                  Ident@20..21
                    Identifier@20..21 "u"
                  Space@21..22 " "
                  Ident@22..23
                    Identifier@22..23 "d"
                  Space@23..24 " "
                  JoinOn@24..35
                    OnKw@24..26 "on"
                    Space@26..27 " "
                    BinaryExpr@27..35
                      IdentExpr@27..28
                        Ident@27..28
                          Identifier@27..28 "a"
                      Space@28..29 " "
                      EqEq@29..31 "=="
                      Space@31..32 " "
                      FieldAccessExpr@32..35
                        IdentExpr@32..33
                          Ident@32..33
                            Identifier@32..33 "d"
                        Dot@33..34 "."
                        Ident@34..35
                          Identifier@34..35 "b"
            "#]],
        );
    }

    #[test]
    fn parse_query_chained() {
        test_support::check(
            "from t |> where a |> select b",
            parse_query,
            expect![[r#"
                SelectExpr@0..29
                  WhereExpr@0..17
                    FromExpr@0..6
                      FromKw@0..4 "from"
                      Space@4..5 " "
                      Ident@5..6
                        Identifier@5..6 "t"
                    Space@6..7 " "
                    Pipe@7..9 "|>"
                    Space@9..10 " "
                    WhereKw@10..15 "where"
                    Space@15..16 " "
                    IdentExpr@16..17
                      Ident@16..17
                        Identifier@16..17 "a"
                  Space@17..18 " "
                  Pipe@18..20 "|>"
                  Space@20..21 " "
                  SelectKw@21..27 "select"
                  Space@27..28 " "
                  SelectItem@28..29
                    IdentExpr@28..29
                      Ident@28..29
                        Identifier@28..29 "b"
            "#]],
        );
    }
}
