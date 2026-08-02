use crate::anf::{JoinCondition, Rel, RelId, SelectItem, Thunk, TreeCopier};
use crate::reduction::{AnfReducer, environment::Environment};

impl AnfReducer<'_> {
    pub(crate) fn reduce_rel(&mut self, id: RelId, env: &mut Environment) -> RelId {
        let input = self.reduced;
        let rel = match input.rel(id) {
            Rel::From { .. } => return self.copy_rel(id),
            Rel::Join {
                left,
                right,
                kind,
                condition,
                ty,
            } => Rel::Join {
                left: self.reduce_rel(*left, env),
                right: self.reduce_rel(*right, env),
                kind: *kind,
                condition: match condition {
                    JoinCondition::On(thunk) => JoinCondition::On(self.reduce_thunk(thunk, env)),
                    JoinCondition::Using(columns) => JoinCondition::Using(columns.clone()),
                },
                ty: *ty,
            },
            Rel::Select {
                input: source,
                items,
                ty,
            } => Rel::Select {
                input: self.reduce_rel(*source, env),
                items: items
                    .iter()
                    .map(|item| self.reduce_select_item(item, env))
                    .collect(),
                ty: *ty,
            },
            Rel::Where {
                input: source,
                predicate,
                ty,
            } => Rel::Where {
                input: self.reduce_rel(*source, env),
                predicate: self.reduce_thunk(predicate, env),
                ty: *ty,
            },
            Rel::Distinct { input: source, ty } => Rel::Distinct {
                input: self.reduce_rel(*source, env),
                ty: *ty,
            },
            Rel::Drop {
                input: source,
                columns,
                ty,
            } => Rel::Drop {
                input: self.reduce_rel(*source, env),
                columns: columns.clone(),
                ty: *ty,
            },
            Rel::Rename {
                input: source,
                items,
                ty,
            } => Rel::Rename {
                input: self.reduce_rel(*source, env),
                items: items.clone(),
                ty: *ty,
            },
            Rel::Extend {
                input: source,
                items,
                ty,
            } => Rel::Extend {
                input: self.reduce_rel(*source, env),
                items: items
                    .iter()
                    .map(|item| self.reduce_select_item(item, env))
                    .collect(),
                ty: *ty,
            },
        };

        self.anf.alloc_rel(rel)
    }

    fn reduce_select_item(&mut self, item: &SelectItem, env: &mut Environment) -> SelectItem {
        SelectItem {
            body: self.reduce_thunk(&item.body, env),
            alias: item.alias,
        }
    }

    fn reduce_thunk(&mut self, thunk: &Thunk, env: &mut Environment) -> Thunk {
        let outer_materialized = std::mem::take(&mut self.materialized);
        let mut out = Vec::new();
        self.reduce_block(&thunk.stmts, env, &mut out);
        let value = self.reduce_atom(thunk.value, env, &mut out);
        self.materialized = outer_materialized;
        Thunk {
            stmts: out.into_boxed_slice(),
            value,
        }
    }
}

#[cfg(test)]
mod tests {
    use expect_test::expect;

    use crate::reduction::test_support::{TABLE, check};

    #[test]
    fn folds_query_column_constant() {
        check(
            &format!("{TABLE}let q = from t |> select a, 1 + 2 as three"),
            expect![[r#"
                struct Row { a, b }
                table t
                let q = from t
                  |> select a, 3i64 as three
            "#]],
        );
    }

    #[test]
    fn keeps_runtime_predicate() {
        check(
            &format!("{TABLE}let q = from t |> where a > 0 |> select a"),
            expect![[r#"
                struct Row { a, b }
                table t
                let q = from t
                  |> where gt(a, 0i32)
                  |> select a
            "#]],
        );
    }

    const JOIN_TABLES: &str = "struct Other { a: int32, c: int32 }\ntable u = Other\nstruct Codes { c: int32, d: int32 }\ntable v = Codes\n";

    #[test]
    fn folds_constant_in_join_condition() {
        check(
            &format!("{TABLE}{JOIN_TABLES}let q = from t |> join v as x on b == x.c + (1 + 1)"),
            expect![[r#"
                struct Row { a, b }
                table t
                struct Other { a, c }
                table u
                struct Codes { c, d }
                table v
                let q = from t
                  |> inner join from v as x on %r0 = add(c, 2i32); %r1 = eq(b, %r0); %r1
            "#]],
        );
    }

    #[test]
    fn keeps_join_using_columns() {
        check(
            &format!("{TABLE}{JOIN_TABLES}let q = from t |> right join u using (a)"),
            expect![[r#"
                struct Row { a, b }
                table t
                struct Other { a, c }
                table u
                struct Codes { c, d }
                table v
                let q = from t
                  |> right join from u using a
            "#]],
        );
    }

    #[test]
    fn list_used_whole_across_two_thunks_is_self_contained() {
        check(
            &format!("{TABLE}let xs = [1, 2]\nfrom t |> where a in xs |> where b in xs"),
            expect![[r#"
                struct Row { a, b }
                table t
                from t
                  |> where xs = [1i32, 2i32]; %r0 = in(a, xs); %r0
                  |> where xs = [1i32, 2i32]; %r1 = in(b, xs); %r1
            "#]],
        );
    }
}
