use crate::anf::{Rel, RelId, SelectItem, Thunk, TreeCopier};
use crate::reduction::{AnfReducer, environment::Environment};

impl AnfReducer<'_> {
    pub(crate) fn reduce_rel(&mut self, id: RelId, env: &mut Environment) -> RelId {
        let input = self.reduced;
        let rel = match input.rel(id) {
            Rel::From { .. } => return self.copy_rel(id),
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
                  |> select %t0.a, 3i64 as three
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
                  |> where gt(%t0.a, 0i32)
                  |> select %t0.a
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
                  |> where xs = [1i32, 2i32]; %r0 = in(%t0.a, xs); %r0
                  |> where xs = [1i32, 2i32]; %r1 = in(%t0.b, xs); %r1
            "#]],
        );
    }
}
