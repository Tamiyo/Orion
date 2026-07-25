use std::collections::{HashMap, HashSet};

use yuzu_core::adt::StringInterner;
use yuzu_diagnostics::{
    diagnostics::{Span, builder::DiagnosticBuilder, engine::DiagnosticsEngine},
    source_map::SourceId,
};
use yuzu_types::{InferKind, SymbolId, Type, TypeCtx, TypeId};

use crate::{
    Expr, ExprId, FuncParam, HirCtx, HirSourceMap, Ident, JoinCondition, Literal, Mutability, Op,
    Rel, RelId, RenameItem, Root, SelectItem, Stmt, StmtId, StructField, StructFieldInit,
    TypeAnnotation, TypeAnnotationId,
    infer::{
        InferCtx,
        symbols::{Binding, ScopeKind, SymbolTable},
    },
};

pub(crate) struct TypeInferrer<'i> {
    hir: &'i HirCtx,
    infer: InferCtx<'i>,
    symbols: SymbolTable,
    interner: &'i mut StringInterner,
    diagnostics: &'i mut DiagnosticsEngine,
    source_map: &'i HirSourceMap,
    source_id: SourceId,
    /// The row struct of the query stage being typed, so bare column names and
    /// `alias.field` resolve against it. `None` outside a query.
    current_row: Option<TypeId>,
    /// Where each alias's columns sit in the current row, so a qualified
    /// `rename` names one column rather than every one that shares its name.
    alias_columns: HashMap<SymbolId, Vec<Option<u32>>>,
}

impl<'i> TypeInferrer<'i> {
    pub(crate) fn new(
        hir: &'i HirCtx,
        types: &'i mut TypeCtx,
        interner: &'i mut StringInterner,
        diagnostics: &'i mut DiagnosticsEngine,
        source_map: &'i HirSourceMap,
        source_id: SourceId,
    ) -> Self {
        Self {
            hir,
            infer: InferCtx::new(types),
            symbols: SymbolTable::new(),
            interner,
            diagnostics,
            source_map,
            source_id,
            current_row: None,
            alias_columns: HashMap::new(),
        }
    }

    pub(crate) fn run(mut self, root: &Root) -> InferCtx<'i> {
        self.hoist_and_infer(&root.stmts);
        self.infer
    }

    /// Registers every declaration in a statement list (so references can be
    /// forward), then types each statement.
    fn hoist_and_infer(&mut self, stmts: &[StmtId]) {
        for &stmt_id in stmts {
            if let Stmt::Struct { name, fields } = self.hir.stmt(stmt_id) {
                self.register_struct(stmt_id, name.symbol, fields);
            }
        }

        for &stmt_id in stmts {
            match self.hir.stmt(stmt_id) {
                Stmt::Table { name, row } => self.register_table(stmt_id, name.symbol, row.symbol),
                Stmt::InlineTable { name, fields } => {
                    self.register_inline_table(stmt_id, name.symbol, fields)
                }
                _ => {}
            }
        }

        for &stmt_id in stmts {
            if let Stmt::Func {
                name,
                params,
                ret_type_annotation,
                ..
            } = self.hir.stmt(stmt_id)
            {
                self.register_func(stmt_id, name.symbol, params, *ret_type_annotation);
            }
        }

        for &stmt_id in stmts {
            self.infer_stmt(stmt_id);
        }
    }

    fn register_struct(&mut self, stmt_id: StmtId, name: SymbolId, fields: &[StructField]) {
        let field_tys = self.resolve_fields(fields);
        let struct_ty = self.infer.types.struct_ty(name, field_tys);
        self.infer.bind_stmt_ty(stmt_id, struct_ty);
        self.symbols.bind_type(name, struct_ty);
        self.symbols.bind_struct(name, stmt_id);
    }

    fn register_table(&mut self, stmt_id: StmtId, name: SymbolId, row: SymbolId) {
        let row_ty = match self.symbols.lookup_type(row) {
            Some(ty) if matches!(self.infer.types.ty(ty), Type::Struct(_)) => ty,
            _ => {
                let message = format!("`{}` is not a struct", self.interner.text(row));
                self.report_stmt(stmt_id, message);
                return;
            }
        };
        let relation_ty = self.infer.types.relation_ty(row_ty);
        self.infer.bind_stmt_ty(stmt_id, relation_ty);
        self.symbols.bind_relation(name, relation_ty);
    }

    fn register_inline_table(&mut self, stmt_id: StmtId, name: SymbolId, fields: &[StructField]) {
        let field_tys = self.resolve_fields(fields);
        let row_ty = self.infer.types.struct_ty(name, field_tys);
        let relation_ty = self.infer.types.relation_ty(row_ty);
        self.infer.bind_stmt_ty(stmt_id, relation_ty);
        self.symbols.bind_relation(name, relation_ty);
    }

    fn resolve_fields(&mut self, fields: &[StructField]) -> Vec<(SymbolId, TypeId)> {
        fields
            .iter()
            .map(|field| {
                (
                    field.name.symbol,
                    self.resolve_annotation(field.type_annotation),
                )
            })
            .collect()
    }

    fn register_func(
        &mut self,
        stmt_id: StmtId,
        name: SymbolId,
        params: &[FuncParam],
        ret_annotation: TypeAnnotationId,
    ) {
        let arg_tys: Vec<TypeId> = params
            .iter()
            .map(|param| self.resolve_annotation(param.type_annotation))
            .collect();

        let ret_ty = self.resolve_return_annotation(ret_annotation);
        let func_ty = self.infer.types.func_ty(arg_tys, ret_ty);
        self.infer.bind_stmt_ty(stmt_id, func_ty);

        self.symbols.bind(
            name,
            Binding::FuncStmt {
                stmt: stmt_id,
                ty: func_ty,
            },
        );
    }

    fn infer_stmt(&mut self, stmt_id: StmtId) {
        match self.hir.stmt(stmt_id) {
            Stmt::Struct { .. } => {}
            Stmt::Impl { .. } => todo!(),
            Stmt::Trait { .. } => todo!(),
            Stmt::Func {
                params,
                ret_type_annotation,
                body,
                ..
            } => self.infer_func_stmt(params, *ret_type_annotation, *body),
            Stmt::Block { stmts } => self.infer_block_stmt(stmts),
            Stmt::Table { .. } | Stmt::InlineTable { .. } => {}
            Stmt::Let {
                name,
                mutability,
                type_annotation,
                expr,
            } => self.infer_let_stmt(stmt_id, name.symbol, *mutability, *type_annotation, *expr),
            Stmt::Assign { target, value } => self.infer_assign_stmt(stmt_id, *target, *value),
            Stmt::Return { expr } => self.infer_return_stmt(stmt_id, *expr),
            Stmt::Expr { expr } => self.infer_expr_stmt(*expr),
            Stmt::Missing => {}
        }
    }

    fn infer_block_stmt(&mut self, stmts: &[StmtId]) {
        self.symbols.push_scope(ScopeKind::Block);
        self.hoist_and_infer(stmts);
        self.symbols.pop_scope();
    }

    fn infer_func_stmt(
        &mut self,
        params: &[FuncParam],
        ret_annotation: TypeAnnotationId,
        body: Option<StmtId>,
    ) {
        let return_ty = self.resolve_return_annotation(ret_annotation);
        self.symbols.push_scope(ScopeKind::Func { return_ty });
        for param in params {
            let ty = self.resolve_annotation(param.type_annotation);
            self.symbols.bind(
                param.name.symbol,
                Binding::Param {
                    symbol: param.name.symbol,
                    ty,
                },
            );
        }
        if let Some(body) = body {
            self.infer_stmt(body);
        }
        self.symbols.pop_scope();
    }

    fn infer_let_stmt(
        &mut self,
        stmt_id: StmtId,
        name: SymbolId,
        mutability: Mutability,
        type_annotation: Option<TypeAnnotationId>,
        expr: ExprId,
    ) {
        let expr_ty_id = self.infer_expr(expr);

        let ty_id = if let Some(annotation) = type_annotation {
            let target_ty_id = self.resolve_annotation(annotation);
            if !self.infer.unify(expr_ty_id, target_ty_id)
                && !self.infer.coerce(expr, expr_ty_id, target_ty_id)
            {
                let resolved_expr_ty = self.infer.resolve(expr_ty_id);
                let expected_expr_ty = self.infer.resolve(target_ty_id);
                let message = format!(
                    "value of type `{:?}` is not assignable to `{:?}`",
                    self.infer.types.ty(resolved_expr_ty),
                    self.infer.types.ty(expected_expr_ty),
                );
                self.report_stmt(stmt_id, message);
            }
            target_ty_id
        } else {
            expr_ty_id
        };

        self.symbols.bind(
            name,
            Binding::LetStmt {
                stmt: stmt_id,
                mutability,
                ty: ty_id,
            },
        );
    }

    fn infer_assign_stmt(&mut self, stmt_id: StmtId, target: ExprId, value: ExprId) {
        let target_ty = self.infer_expr(target);
        let value_ty = self.infer_expr(value);

        self.check_assignable(stmt_id, target);

        if !self.infer.unify(value_ty, target_ty) && !self.infer.coerce(value, value_ty, target_ty)
        {
            let resolved_value = self.infer.resolve(value_ty);
            let resolved_target = self.infer.resolve(target_ty);
            let message = format!(
                "value of type `{:?}` is not assignable to `{:?}`",
                self.infer.types.ty(resolved_value),
                self.infer.types.ty(resolved_target),
            );
            self.report_stmt(stmt_id, message);
        }
    }

    fn check_assignable(&mut self, stmt_id: StmtId, target: ExprId) {
        match self.hir.expr(target) {
            Expr::Ident { value } => {
                let name = value.symbol;
                match self.symbols.lookup(name).copied() {
                    Some(Binding::LetStmt {
                        mutability: Mutability::Mutable,
                        ..
                    })
                    | None => {}
                    Some(Binding::LetStmt {
                        mutability: Mutability::Immutable,
                        ..
                    }) => {
                        let message = format!(
                            "cannot assign to immutable variable `{}`",
                            self.interner.text(name)
                        );
                        self.report_stmt(stmt_id, message);
                    }
                    Some(_) => {
                        let message = format!("cannot assign to `{}`", self.interner.text(name));
                        self.report_stmt(stmt_id, message);
                    }
                }
            }
            Expr::FieldAccess { base, field } => {
                self.check_field_mutable(stmt_id, *base, field.symbol);
                self.check_place(stmt_id, *base);
            }
            _ => self.report_stmt(stmt_id, "cannot assign to this expression"),
        }
    }

    fn check_place(&mut self, stmt_id: StmtId, target: ExprId) {
        match self.hir.expr(target) {
            Expr::Ident { .. } => {}
            Expr::FieldAccess { base, .. } => self.check_place(stmt_id, *base),
            _ => self.report_stmt(stmt_id, "cannot assign to this expression"),
        }
    }

    fn check_field_mutable(&mut self, stmt_id: StmtId, base: ExprId, field: SymbolId) {
        let Some(base_ty) = self.infer.expr_ty(base) else {
            return;
        };
        let base_ty = self.infer.resolve(base_ty);
        let struct_name = match self.infer.types.ty(base_ty) {
            Type::Struct(s) => s.name,
            _ => return,
        };
        if self.field_is_immutable(struct_name, field) {
            let message = format!(
                "cannot assign to immutable field `{}`",
                self.interner.text(field)
            );
            self.report_stmt(stmt_id, message);
        }
    }

    fn field_is_immutable(&self, struct_name: SymbolId, field: SymbolId) -> bool {
        let Some(decl) = self.symbols.lookup_struct(struct_name) else {
            return false;
        };
        let Stmt::Struct { fields, .. } = self.hir.stmt(decl) else {
            return false;
        };
        fields
            .iter()
            .find(|f| f.name.symbol == field)
            .is_some_and(|f| f.mutability == Mutability::Immutable)
    }

    fn infer_return_stmt(&mut self, stmt_id: StmtId, expr: Option<ExprId>) {
        let value_ty = match expr {
            Some(expr) => self.infer_expr(expr),
            None => self.infer.types.unit_ty(),
        };

        let Some(return_ty) = self.symbols.return_ty() else {
            return;
        };

        // If value_ty can be unified to return_ty, we can return.
        if self.infer.unify(value_ty, return_ty) {
            return;
        }

        if let Some(expr) = expr
            && self.infer.coerce(expr, value_ty, return_ty)
        {
            return;
        }

        let resolved_value = self.infer.resolve(value_ty);
        let resolved_return = self.infer.resolve(return_ty);
        let message = format!(
            "value of type `{:?}` is not assignable to return type `{:?}`",
            self.infer.types.ty(resolved_value),
            self.infer.types.ty(resolved_return),
        );
        self.report_stmt(stmt_id, message);
    }

    fn infer_expr_stmt(&mut self, expr: ExprId) {
        self.infer_expr(expr);
    }

    fn infer_rel(&mut self, id: RelId) -> TypeId {
        let ty = match self.hir.rel(id).clone() {
            Rel::From { relation, alias } => self.infer_from_rel(id, relation.symbol, alias),
            Rel::Join {
                left,
                right,
                condition,
                ..
            } => self.infer_join_rel(id, left, right, &condition),
            Rel::Select { input, items } => self.infer_select_rel(input, &items),
            Rel::Where { input, predicate } => self.infer_where_rel(input, predicate),
            Rel::Distinct { input } => self.infer_rel(input),
            Rel::Drop { input, items } => self.infer_drop_rel(input, &items),
            Rel::Rename { input, items } => self.infer_rename_rel(id, input, &items),
            Rel::Extend { input, items } => self.infer_extend_rel(input, &items),
            Rel::Missing => self.infer.types.error_ty(),
        };
        self.infer.bind_rel_ty(id, ty)
    }

    fn infer_from_rel(&mut self, id: RelId, relation: SymbolId, alias: Option<Ident>) -> TypeId {
        let Some(relation_ty) = self.symbols.lookup_relation(relation) else {
            let message = format!("`{}` is not a table", self.interner.text(relation));
            return self.error_rel(id, message);
        };

        // Bind the row alias so later stages can reference `alias.field`.
        if let (Some(alias), Some(row)) = (alias, self.row_of(relation_ty)) {
            self.symbols.bind(
                alias.symbol,
                Binding::Param {
                    symbol: alias.symbol,
                    ty: row,
                },
            );
            let width = self.row_field_types(row).map_or(0, |fields| fields.len());
            self.alias_columns
                .insert(alias.symbol, (0..width as u32).map(Some).collect());
        }
        relation_ty
    }

    fn infer_join_rel(
        &mut self,
        id: RelId,
        left: RelId,
        right: RelId,
        condition: &JoinCondition,
    ) -> TypeId {
        let left_ty = self.infer_rel(left);
        let right_ty = self.infer_rel(right);
        let (Some(left_row), Some(right_row)) = (self.row_of(left_ty), self.row_of(right_ty))
        else {
            return self.infer.types.error_ty();
        };

        let (Some(left_fields), Some(right_fields)) = (
            self.row_field_types(left_row),
            self.row_field_types(right_row),
        ) else {
            return self.infer.types.error_ty();
        };

        let fields = match condition {
            JoinCondition::On(expr) => {
                let joined = [left_fields.clone(), right_fields.clone()].concat();
                self.infer_join_condition(*expr, joined.clone());
                joined
            }
            JoinCondition::Using(columns) => {
                match self.infer_join_using(id, columns, &left_fields, &right_fields) {
                    Some(fields) => fields,
                    None => return self.infer.types.error_ty(),
                }
            }
        };

        self.place_joined_alias(right, &left_fields, &right_fields, condition);
        let out_row = self.anonymous_row(fields);
        self.infer.types.relation_ty(out_row)
    }

    /// The joined relation's columns follow the left input's, except a `using`
    /// key, which is carried once from the left.
    fn place_joined_alias(
        &mut self,
        right: RelId,
        left: &[(SymbolId, TypeId)],
        right_fields: &[(SymbolId, TypeId)],
        condition: &JoinCondition,
    ) {
        let Rel::From {
            alias: Some(alias), ..
        } = self.hir.rel(right)
        else {
            return;
        };
        let alias = alias.symbol;

        let keys: Vec<SymbolId> = match condition {
            JoinCondition::On(_) => Vec::new(),
            JoinCondition::Using(columns) => columns.iter().map(|column| column.symbol).collect(),
        };

        let mut placement = Vec::new();
        let mut next = left.len() as u32;
        for &(name, _) in right_fields {
            if keys.contains(&name) {
                let carried = left.iter().position(|&(field, _)| field == name);
                placement.push(carried.map(|index| index as u32));
                continue;
            }
            placement.push(Some(next));
            next += 1;
        }
        self.alias_columns.insert(alias, placement);
    }

    fn infer_join_condition(&mut self, expr: ExprId, fields: Vec<(SymbolId, TypeId)>) {
        let row = self.anonymous_row(fields);
        let saved = self.current_row.replace(row);
        let condition_ty = self.infer_expr(expr);
        self.current_row = saved;
        self.check_bool(expr, condition_ty, "`on` condition");
    }

    fn infer_join_using(
        &mut self,
        id: RelId,
        columns: &[Ident],
        left: &[(SymbolId, TypeId)],
        right: &[(SymbolId, TypeId)],
    ) -> Option<Vec<(SymbolId, TypeId)>> {
        let mut usable = true;
        for column in columns {
            if !self.check_using_column(id, column.symbol, left, right) {
                usable = false;
            }
        }
        if !usable {
            return None;
        }

        let keys: HashSet<SymbolId> = columns.iter().map(|column| column.symbol).collect();
        Some(
            left.iter()
                .chain(right.iter().filter(|(name, _)| !keys.contains(name)))
                .copied()
                .collect(),
        )
    }

    fn check_using_column(
        &mut self,
        id: RelId,
        name: SymbolId,
        left: &[(SymbolId, TypeId)],
        right: &[(SymbolId, TypeId)],
    ) -> bool {
        let Some(left_ty) = field_ty(left, name) else {
            let message = format!(
                "`using` column `{}` is not in the join's left input",
                self.interner.text(name)
            );
            self.report_rel(id, message);
            return false;
        };
        let Some(right_ty) = field_ty(right, name) else {
            let message = format!(
                "`using` column `{}` is not in the joined relation",
                self.interner.text(name)
            );
            self.report_rel(id, message);
            return false;
        };
        if self.infer.resolve(left_ty) != self.infer.resolve(right_ty) {
            let message = format!(
                "`using` column `{}` is `{:?}` on the left and `{:?}` on the right",
                self.interner.text(name),
                self.infer.types.ty(left_ty),
                self.infer.types.ty(right_ty),
            );
            self.report_rel(id, message);
            return false;
        }
        true
    }

    fn check_bool(&mut self, expr: ExprId, ty: TypeId, subject: &str) {
        let ty = self.infer.resolve(ty);
        let bool_ty = self.infer.types.bool_ty();
        let error_ty = self.infer.types.error_ty();
        if ty == bool_ty || ty == error_ty {
            return;
        }

        let message = format!(
            "{subject} must be `bool`, found `{:?}`",
            self.infer.types.ty(ty)
        );
        self.report_expr(expr, message);
    }

    fn infer_select_rel(&mut self, input: RelId, items: &[SelectItem]) -> TypeId {
        let input_ty = self.infer_rel(input);
        let Some(row) = self.row_of(input_ty) else {
            return self.infer.types.error_ty();
        };

        let saved = self.current_row.replace(row);
        let mut fields = Vec::with_capacity(items.len());
        let mut anonymous = 0;
        for item in items {
            let col_ty = self.infer_expr(item.expr);
            let name = self.column_name(item, &mut anonymous);
            fields.push((name, col_ty));
        }
        self.current_row = saved;

        let out_row = self.anonymous_row(fields);
        self.infer.types.relation_ty(out_row)
    }

    fn infer_where_rel(&mut self, input: RelId, predicate: ExprId) -> TypeId {
        let input_ty = self.infer_rel(input);
        let Some(row) = self.row_of(input_ty) else {
            self.infer_expr(predicate);
            return self.infer.types.error_ty();
        };

        let saved = self.current_row.replace(row);
        let pred_ty = self.infer_expr(predicate);
        self.current_row = saved;
        self.check_bool(predicate, pred_ty, "`where` predicate");

        // A filter keeps the input schema.
        input_ty
    }

    fn infer_drop_rel(&mut self, input: RelId, columns: &[Ident]) -> TypeId {
        let input_ty = self.infer_rel(input);
        let Some(row) = self.row_of(input_ty) else {
            return self.infer.types.error_ty();
        };

        let dropped: HashSet<SymbolId> = columns.iter().map(|column| column.symbol).collect();
        let fields: Vec<(SymbolId, TypeId)> = match self.infer.types.ty(row) {
            Type::Struct(s) => s
                .fields
                .iter()
                .filter(|(name, _)| !dropped.contains(name))
                .copied()
                .collect(),
            _ => return self.infer.types.error_ty(),
        };

        let out_row = self.anonymous_row(fields);
        self.infer.types.relation_ty(out_row)
    }

    fn infer_rename_rel(&mut self, id: RelId, input: RelId, items: &[RenameItem]) -> TypeId {
        let input_ty = self.infer_rel(input);
        let Some(row) = self.row_of(input_ty) else {
            return self.infer.types.error_ty();
        };

        let Some(mut fields) = self.row_field_types(row) else {
            return self.infer.types.error_ty();
        };

        for item in items {
            let Some(column) = self.rename_target(id, item, &fields) else {
                return self.infer.types.error_ty();
            };
            fields[column as usize].0 = item.to.symbol;
        }

        let out_row = self.anonymous_row(fields);
        self.infer.types.relation_ty(out_row)
    }

    /// The one column a rename item names: a qualified item goes through its
    /// alias, a bare one has to match exactly one column of the row.
    fn rename_target(
        &mut self,
        id: RelId,
        item: &RenameItem,
        fields: &[(SymbolId, TypeId)],
    ) -> Option<u32> {
        let name = item.from.symbol;
        let Some(alias) = item.qualifier else {
            let mut matches = fields
                .iter()
                .enumerate()
                .filter(|(_, (field, _))| *field == name);
            return match (matches.next(), matches.next()) {
                (Some((column, _)), None) => Some(column as u32),
                (Some(_), Some(_)) => {
                    let message = format!(
                        "column `{}` is ambiguous; qualify it with a relation alias",
                        self.interner.text(name)
                    );
                    self.report_rel(id, message);
                    None
                }
                _ => {
                    let message =
                        format!("column `{}` is not in this row", self.interner.text(name));
                    self.report_rel(id, message);
                    None
                }
            };
        };

        let column = self
            .symbols
            .lookup(alias.symbol)
            .map(|binding| binding.ty())
            .and_then(|row| self.row_field_types(row))
            .and_then(|row| row.iter().position(|&(field, _)| field == name))
            .and_then(|index| self.alias_columns.get(&alias.symbol)?.get(index).copied())
            .flatten();

        if column.is_none() {
            let message = format!(
                "`{}` has no column `{}` here",
                self.interner.text(alias.symbol),
                self.interner.text(name)
            );
            self.report_rel(id, message);
        }
        column
    }

    fn infer_extend_rel(&mut self, input: RelId, items: &[SelectItem]) -> TypeId {
        let input_ty = self.infer_rel(input);
        let Some(row) = self.row_of(input_ty) else {
            return self.infer.types.error_ty();
        };

        let mut fields: Vec<(SymbolId, TypeId)> = match self.infer.types.ty(row) {
            Type::Struct(s) => s.fields.clone(),
            _ => return self.infer.types.error_ty(),
        };

        let saved = self.current_row.replace(row);
        let mut anonymous = 0;
        for item in items {
            let col_ty = self.infer_expr(item.expr);
            let name = self.column_name(item, &mut anonymous);
            fields.push((name, col_ty));
        }
        self.current_row = saved;

        let out_row = self.anonymous_row(fields);
        self.infer.types.relation_ty(out_row)
    }

    /// The output column name: the `as` alias, else a bare identifier's or field
    /// access's own name, else a generated `%gN` for an anonymous expression.
    fn column_name(&mut self, item: &SelectItem, anonymous: &mut usize) -> SymbolId {
        if let Some(alias) = item.alias {
            return alias.symbol;
        }
        match self.hir.expr(item.expr) {
            Expr::Ident { value } => value.symbol,
            Expr::FieldAccess { field, .. } => field.symbol,
            _ => {
                let name = format!("%g{anonymous}");
                *anonymous += 1;
                self.interner.intern(&name)
            }
        }
    }

    fn anonymous_row(&mut self, fields: Vec<(SymbolId, TypeId)>) -> TypeId {
        let name = self.interner.intern("");
        self.infer.types.struct_ty(name, fields)
    }

    fn row_field_types(&self, row: TypeId) -> Option<Vec<(SymbolId, TypeId)>> {
        match self.infer.types.ty(row) {
            Type::Struct(s) => Some(s.fields.clone()),
            _ => None,
        }
    }

    fn row_of(&self, relation_ty: TypeId) -> Option<TypeId> {
        match self.infer.types.ty(relation_ty) {
            Type::Relation(relation) => Some(relation.inner),
            _ => None,
        }
    }

    /// A join concatenates its inputs, so a bare name can match more than one
    /// column; that is only an error where it is used, not where it arose.
    fn current_row_field(&self, name: SymbolId) -> ColumnLookup {
        let Some(row) = self.current_row else {
            return ColumnLookup::Absent;
        };
        let Type::Struct(row) = self.infer.types.ty(row) else {
            return ColumnLookup::Absent;
        };

        let mut matches = row.fields.iter().filter(|(n, _)| *n == name);
        match (matches.next(), matches.next()) {
            (Some(&(_, ty)), None) => ColumnLookup::Unique(ty),
            (Some(_), Some(_)) => ColumnLookup::Ambiguous,
            _ => ColumnLookup::Absent,
        }
    }

    fn error_rel(&mut self, id: RelId, message: impl Into<String>) -> TypeId {
        self.report_rel(id, message);
        self.infer.types.error_ty()
    }

    fn report_rel(&mut self, id: RelId, message: impl Into<String>) {
        let range = self
            .source_map
            .rel(id)
            .expect("a reported node is always in the source map")
            .text_range();
        let span = Span {
            source_id: self.source_id,
            range,
        };
        self.diagnostics
            .emit(DiagnosticBuilder::error(span, message));
    }

    fn infer_expr(&mut self, expr_id: ExprId) -> TypeId {
        match self.hir.expr(expr_id) {
            Expr::Ident { value } => self.infer_ident_expr(expr_id, value),
            Expr::Call { op, args } => self.infer_call_expr(expr_id, *op, args),
            Expr::FuncCall { callee, args } => self.infer_func_call_expr(expr_id, *callee, args),
            Expr::MethodCall { .. } => todo!(),
            Expr::FieldAccess { base, field } => {
                self.infer_field_access_expr(expr_id, *base, field.symbol)
            }
            Expr::StructInit { name, fields } => {
                self.infer_struct_init_expr(expr_id, *name, fields)
            }
            Expr::ListInit { elements } => self.infer_list_init_expr(expr_id, elements),
            Expr::Literal(literal) => self.infer_literal_expr(expr_id, *literal),
            Expr::Rel(rel) => {
                self.symbols.push_scope(ScopeKind::Block);
                let ty = self.infer_rel(*rel);
                self.symbols.pop_scope();
                self.infer.bind_expr_ty(expr_id, ty)
            }
            Expr::Missing => self.poison(expr_id),
        }
    }

    fn infer_ident_expr(&mut self, expr_id: ExprId, value: &Ident) -> TypeId {
        let name = value.symbol;
        if let Some(ty) = self.symbols.lookup(name).map(|binding| binding.ty()) {
            return self.infer.bind_expr_ty(expr_id, ty);
        }

        // Inside a query, a bare name may be a column of the current row.
        match self.current_row_field(name) {
            ColumnLookup::Unique(ty) => return self.infer.bind_expr_ty(expr_id, ty),
            ColumnLookup::Ambiguous => {
                let message = format!(
                    "column `{}` is ambiguous; qualify it with a relation alias",
                    self.interner.text(name)
                );
                return self.error_expr(expr_id, message);
            }
            ColumnLookup::Absent => {}
        }
        let message = format!("unresolved identifier `{}`", self.interner.text(name));
        self.error_expr(expr_id, message)
    }

    fn infer_call_expr(&mut self, expr_id: ExprId, op: Op, args: &[ExprId]) -> TypeId {
        for &arg in args {
            self.infer_expr(arg);
        }

        let error_ty = self.infer.types.error_ty();
        let arg_tys: Vec<TypeId> = args
            .iter()
            .map(|&arg| self.infer.expr_ty(arg).unwrap_or(error_ty))
            .collect();

        if arg_tys.contains(&error_ty) {
            return self.poison(expr_id);
        }

        let ty = op.resolve(&arg_tys, &mut self.infer);
        if ty == error_ty {
            let operands: Vec<String> = arg_tys
                .iter()
                .map(|&arg| format!("`{}`", self.type_name(arg)))
                .collect();
            self.report_expr(
                expr_id,
                format!(
                    "operator `{}` cannot be applied to {}",
                    op.symbol(),
                    operands.join(" and ")
                ),
            );
        }
        self.infer.bind_expr_ty(expr_id, ty)
    }

    /// A type rendered for a diagnostic: scalars by name, compounds
    /// structurally (`List[Int64]`, a struct by its declared name).
    fn type_name(&mut self, ty: TypeId) -> String {
        let resolved = self.infer.resolve(ty);
        match self.infer.types.ty(resolved).clone() {
            Type::List(list) => format!("List[{}]", self.type_name(list.inner)),
            Type::Relation(relation) => format!("Relation[{}]", self.type_name(relation.inner)),
            Type::Struct(row) => self.interner.text(row.name).to_string(),
            other => format!("{other:?}"),
        }
    }

    fn infer_func_call_expr(&mut self, expr_id: ExprId, callee: ExprId, args: &[ExprId]) -> TypeId {
        let callee_ty = self.infer_expr(callee);
        let callee_ty = self.infer.resolve(callee_ty);
        let error_ty = self.infer.types.error_ty();

        let Type::Func(func) = self.infer.types.ty(callee_ty).clone() else {
            for &arg in args {
                self.infer_expr(arg);
            }
            if callee_ty == error_ty {
                return self.infer.bind_expr_ty(expr_id, error_ty);
            }
            let message = format!(
                "type `{:?}` is not callable",
                self.infer.types.ty(callee_ty)
            );
            return self.error_expr(expr_id, message);
        };

        for (index, &arg) in args.iter().enumerate() {
            let arg_ty = self.infer_expr(arg);
            let Some(&param_ty) = func.args.get(index) else {
                continue;
            };

            if !self.is_assignable(arg, arg_ty, param_ty) {
                let resolved_arg = self.infer.resolve(arg_ty);
                let resolved_param = self.infer.resolve(param_ty);
                let message = format!(
                    "argument of type `{:?}` is not assignable to parameter of type `{:?}`",
                    self.infer.types.ty(resolved_arg),
                    self.infer.types.ty(resolved_param),
                );
                self.report_expr(arg, message);
            }
        }

        if args.len() != func.args.len() {
            let message = format!(
                "expected {} argument(s), found {}",
                func.args.len(),
                args.len()
            );
            self.report_expr(expr_id, message);
        }

        self.infer.bind_expr_ty(expr_id, func.ret_type)
    }

    fn is_assignable(&mut self, expr: ExprId, value_ty: TypeId, target_ty: TypeId) -> bool {
        self.infer.unify(value_ty, target_ty) || self.infer.coerce(expr, value_ty, target_ty)
    }

    fn infer_field_access_expr(
        &mut self,
        expr_id: ExprId,
        base: ExprId,
        field: SymbolId,
    ) -> TypeId {
        let base_ty = self.infer_expr(base);
        let base_ty = self.infer.resolve(base_ty);
        let error_ty = self.infer.types.error_ty();

        // A poisoned base was already reported; don't cascade.
        if base_ty == error_ty {
            return self.infer.bind_expr_ty(expr_id, error_ty);
        }

        let (struct_name, field_ty) = match self.infer.types.ty(base_ty) {
            Type::Struct(s) => (
                Some(s.name),
                s.fields.iter().find(|(n, _)| *n == field).map(|(_, t)| *t),
            ),
            _ => (None, None),
        };

        if let Some(ty) = field_ty {
            return self.infer.bind_expr_ty(expr_id, ty);
        }

        let message = match struct_name {
            Some(struct_name) => format!(
                "struct `{}` has no field `{}`",
                self.interner.text(struct_name),
                self.interner.text(field),
            ),
            None => format!("type `{:?}` has no fields", self.infer.types.ty(base_ty)),
        };
        self.error_expr(expr_id, message)
    }

    fn infer_struct_init_expr(
        &mut self,
        expr_id: ExprId,
        name: SymbolId,
        fields: &[StructFieldInit],
    ) -> TypeId {
        // The name must resolve to a declared struct.
        let struct_ty = self.symbols.lookup_type(name);
        let declared: Option<Vec<(SymbolId, TypeId)>> = struct_ty.and_then(|ty| {
            if let Type::Struct(s) = self.infer.types.ty(ty) {
                Some(s.fields.clone())
            } else {
                None
            }
        });

        let (Some(struct_ty), Some(declared)) = (struct_ty, declared) else {
            for field in fields {
                self.infer_expr(field.value);
            }
            let message = format!("`{}` is not a struct", self.interner.text(name));
            return self.error_expr(expr_id, message);
        };

        // Each initializer must name a declared field exactly once with an
        // assignable value; every declared field must be initialized.
        let mut seen: HashSet<SymbolId> = HashSet::new();
        for field in fields {
            let field_name = field.name.symbol;
            let value_ty = self.infer_expr(field.value);

            let Some(&(_, declared_ty)) = declared.iter().find(|(n, _)| *n == field_name) else {
                let message = format!(
                    "struct `{}` has no field `{}`",
                    self.interner.text(name),
                    self.interner.text(field_name),
                );
                self.error_expr(expr_id, message);
                continue;
            };

            if seen.contains(&field_name) {
                let message = format!(
                    "field `{}` is initialized more than once",
                    self.interner.text(field_name)
                );
                self.error_expr(expr_id, message);
                continue;
            }
            seen.insert(field_name);

            if !self.infer.unify(value_ty, declared_ty)
                && !self.infer.coerce(field.value, value_ty, declared_ty)
            {
                let message = format!(
                    "value is not assignable to field `{}`",
                    self.interner.text(field_name)
                );
                self.error_expr(expr_id, message);
            }
        }

        for (declared_name, _) in &declared {
            if !seen.contains(declared_name) {
                let message = format!(
                    "missing field `{}` in `{}` literal",
                    self.interner.text(*declared_name),
                    self.interner.text(name),
                );
                self.error_expr(expr_id, message);
            }
        }

        self.infer.bind_expr_ty(expr_id, struct_ty)
    }

    fn infer_list_init_expr(&mut self, expr_id: ExprId, elements: &[ExprId]) -> TypeId {
        if elements.is_empty() {
            let var = self.infer.fresh_var(InferKind::General);
            let list_ty = self.infer.types.list_ty(var);
            return self.infer.bind_expr_ty(expr_id, list_ty);
        }

        let inner_ty = self.infer_expr(elements[0]);

        for element in elements.iter().skip(1) {
            let element_ty = self.infer_expr(*element);
            self.infer.unify(inner_ty, element_ty);
        }

        let list_ty = self.infer.types.list_ty(inner_ty);
        self.infer.bind_expr_ty(expr_id, list_ty)
    }

    fn infer_literal_expr(&mut self, expr_id: ExprId, literal: Literal) -> TypeId {
        let ty = match literal {
            Literal::Bool { .. } => self.infer.types.bool_ty(),
            Literal::String { .. } => self.infer.types.str_ty(),
            Literal::Int { .. } => self.infer.fresh_var(InferKind::Int),
            Literal::Float { .. } => self.infer.fresh_var(InferKind::Float),
            Literal::Missing => self.infer.types.error_ty(),
        };
        self.infer.bind_expr_ty(expr_id, ty)
    }

    fn poison(&mut self, expr_id: ExprId) -> TypeId {
        let error = self.infer.types.error_ty();
        self.infer.bind_expr_ty(expr_id, error)
    }

    fn resolve_return_annotation(&mut self, id: TypeAnnotationId) -> TypeId {
        match self.hir.annotation(id) {
            TypeAnnotation::Missing => self.infer.types.unit_ty(),
            _ => self.resolve_annotation(id),
        }
    }

    fn resolve_annotation(&mut self, id: TypeAnnotationId) -> TypeId {
        match self.hir.annotation(id) {
            TypeAnnotation::Named { name, args } => {
                self.resolve_named_annotation(id, name.symbol, args)
            }
            TypeAnnotation::Func { params, ret } => {
                let param_tys: Vec<TypeId> = params
                    .iter()
                    .map(|&param| self.resolve_annotation(param))
                    .collect();
                let ret_ty = self.resolve_annotation(*ret);
                self.infer.types.func_ty(param_tys, ret_ty)
            }
            TypeAnnotation::Self_ | TypeAnnotation::Missing => self.infer.types.error_ty(),
        }
    }

    fn resolve_named_annotation(
        &mut self,
        id: TypeAnnotationId,
        name: SymbolId,
        args: &[TypeAnnotationId],
    ) -> TypeId {
        let name_str = self.interner.text(name).to_string();

        // Container types take exactly one element type.
        if name_str == "List" || name_str == "Relation" {
            if args.len() != 1 {
                return self.infer.types.error_ty();
            }
            let inner = self.resolve_annotation(args[0]);
            return match name_str.as_str() {
                "List" => self.infer.types.list_ty(inner),
                _ => self.infer.types.relation_ty(inner),
            };
        }

        let scalar = match name_str.as_str() {
            "int64" => Type::Int64,
            "int8" => Type::Int8,
            "int16" => Type::Int16,
            "int32" => Type::Int32,
            "uint8" => Type::UInt8,
            "uint16" => Type::UInt16,
            "uint32" => Type::UInt32,
            "uint64" => Type::UInt64,
            "float64" => Type::Float64,
            "float32" => Type::Float32,
            "bool" => Type::Bool,
            "str" => Type::String,
            "unit" => Type::Unit,
            _ => {
                if let Some(ty) = self.symbols.lookup_type(name) {
                    return ty;
                }
                let message = format!("unknown type `{name_str}`");
                self.report_annotation(id, message);
                return self.infer.types.error_ty();
            }
        };

        // A scalar takes no arguments.
        if !args.is_empty() {
            return self.infer.types.error_ty();
        }

        self.infer.types.intern_ty(scalar)
    }

    fn report_stmt(&mut self, id: StmtId, message: impl Into<String>) {
        let range = self
            .source_map
            .stmt(id)
            .expect("a reported node is always in the source map")
            .text_range();

        let span = Span {
            source_id: self.source_id,
            range,
        };

        self.diagnostics
            .emit(DiagnosticBuilder::error(span, message));
    }

    fn report_expr(&mut self, id: ExprId, message: impl Into<String>) {
        let range = self
            .source_map
            .expr(id)
            .expect("a reported node is always in the source map")
            .text_range();

        let span = Span {
            source_id: self.source_id,
            range,
        };
        self.diagnostics
            .emit(DiagnosticBuilder::error(span, message));
    }

    fn error_expr(&mut self, id: ExprId, message: impl Into<String>) -> TypeId {
        self.report_expr(id, message);
        self.poison(id)
    }

    fn report_annotation(&mut self, id: TypeAnnotationId, message: impl Into<String>) {
        let range = self
            .source_map
            .annotation(id)
            .expect("a reported node is always in the source map")
            .text_range();

        let span = Span {
            source_id: self.source_id,
            range,
        };
        self.diagnostics
            .emit(DiagnosticBuilder::error(span, message));
    }
}

fn field_ty(fields: &[(SymbolId, TypeId)], name: SymbolId) -> Option<TypeId> {
    fields
        .iter()
        .find(|(field, _)| *field == name)
        .map(|&(_, ty)| ty)
}

enum ColumnLookup {
    Absent,
    Unique(TypeId),
    Ambiguous,
}

#[cfg(test)]
mod tests {
    use expect_test::expect;
    use yuzu_core::adt::{Int, StringInterner};

    use crate::{
        Expr, FuncParam, HirCtx, Ident, Literal, Mutability, Root, Stmt, StructField,
        StructFieldInit, TypeAnnotation,
        infer::test_support::{check, check_src},
    };

    fn assign_struct_field(
        hir: &mut HirCtx,
        interner: &mut StringInterner,
        field_mutability: Mutability,
    ) -> Root {
        let s_ty = interner.intern("S");
        let f = interner.intern("f");
        let s = interner.intern("s");
        let int64 = interner.intern("int64");

        let int_ann = hir.alloc_annotation(TypeAnnotation::Named {
            name: Ident { symbol: int64 },
            args: Box::new([]),
        });
        let struct_stmt = hir.alloc_stmt(Stmt::Struct {
            name: Ident { symbol: s_ty },
            fields: Box::new([StructField {
                name: Ident { symbol: f },
                mutability: field_mutability,
                type_annotation: int_ann,
            }]),
        });

        let init = hir.alloc_expr(Expr::Literal(Literal::Int {
            value: Int::from(0u64),
        }));
        let struct_init = hir.alloc_expr(Expr::StructInit {
            name: s_ty,
            fields: Box::new([StructFieldInit {
                name: Ident { symbol: f },
                value: init,
            }]),
        });
        let let_stmt = hir.alloc_stmt(Stmt::Let {
            name: Ident { symbol: s },
            mutability: Mutability::Mutable,
            type_annotation: None,
            expr: struct_init,
        });

        let base = hir.alloc_expr(Expr::Ident {
            value: Ident { symbol: s },
        });
        let target = hir.alloc_expr(Expr::FieldAccess {
            base,
            field: Ident { symbol: f },
        });
        let value = hir.alloc_expr(Expr::Literal(Literal::Int {
            value: Int::from(1u64),
        }));
        let assign = hir.alloc_stmt(Stmt::Assign { target, value });

        Root {
            stmts: Box::new([struct_stmt, let_stmt, assign]),
        }
    }

    fn assign_variable(
        hir: &mut HirCtx,
        interner: &mut StringInterner,
        mutability: Mutability,
    ) -> Root {
        let x = interner.intern("x");
        let init = hir.alloc_expr(Expr::Literal(Literal::Int {
            value: Int::from(0u64),
        }));
        let let_stmt = hir.alloc_stmt(Stmt::Let {
            name: Ident { symbol: x },
            mutability,
            type_annotation: None,
            expr: init,
        });
        let target = hir.alloc_expr(Expr::Ident {
            value: Ident { symbol: x },
        });
        let value = hir.alloc_expr(Expr::Literal(Literal::Int {
            value: Int::from(1u64),
        }));
        let assign = hir.alloc_stmt(Stmt::Assign { target, value });

        Root {
            stmts: Box::new([let_stmt, assign]),
        }
    }

    #[test]
    fn mutable_struct_field_is_assignable() {
        check(
            |hir, interner| assign_struct_field(hir, interner, Mutability::Mutable),
            expect![""],
        );
    }

    #[test]
    fn mutable_variable_is_assignable() {
        check(
            |hir, interner| assign_variable(hir, interner, Mutability::Mutable),
            expect![""],
        );
    }

    fn func_return(
        hir: &mut HirCtx,
        interner: &mut StringInterner,
        param: &str,
        ret: &str,
    ) -> Root {
        let f = interner.intern("f");
        let x = interner.intern("x");
        let param = interner.intern(param);
        let ret = interner.intern(ret);

        let param_ann = hir.alloc_annotation(TypeAnnotation::Named {
            name: Ident { symbol: param },
            args: Box::new([]),
        });
        let ret_ann = hir.alloc_annotation(TypeAnnotation::Named {
            name: Ident { symbol: ret },
            args: Box::new([]),
        });

        let x_ref = hir.alloc_expr(Expr::Ident {
            value: Ident { symbol: x },
        });
        let return_stmt = hir.alloc_stmt(Stmt::Return { expr: Some(x_ref) });
        let body = hir.alloc_stmt(Stmt::Block {
            stmts: Box::new([return_stmt]),
        });

        let func = hir.alloc_stmt(Stmt::Func {
            name: Ident { symbol: f },
            type_params: Box::new([]),
            params: Box::new([FuncParam {
                name: Ident { symbol: x },
                type_annotation: param_ann,
            }]),
            type_bounds: Box::new([]),
            ret_type_annotation: ret_ann,
            body: Some(body),
        });

        Root {
            stmts: Box::new([func]),
        }
    }

    #[test]
    fn return_matching_type_is_ok() {
        check(
            |hir, interner| func_return(hir, interner, "int64", "int64"),
            expect![""],
        );
    }

    #[test]
    fn return_widens_to_return_type() {
        check(
            |hir, interner| func_return(hir, interner, "int32", "int64"),
            expect![""],
        );
    }

    fn int64_field(hir: &mut HirCtx, interner: &mut StringInterner, name: &str) -> StructField {
        let name = interner.intern(name);
        let int64 = interner.intern("int64");
        let annotation = hir.alloc_annotation(TypeAnnotation::Named {
            name: Ident { symbol: int64 },
            args: Box::new([]),
        });
        StructField {
            name: Ident { symbol: name },
            mutability: Mutability::Immutable,
            type_annotation: annotation,
        }
    }

    fn named_table(hir: &mut HirCtx, interner: &mut StringInterner, row: &str) -> Root {
        let struct_name = interner.intern("Row");
        let table = interner.intern("T");
        let row = interner.intern(row);

        let field = int64_field(hir, interner, "x");
        let struct_stmt = hir.alloc_stmt(Stmt::Struct {
            name: Ident {
                symbol: struct_name,
            },
            fields: Box::new([field]),
        });
        let table_stmt = hir.alloc_stmt(Stmt::Table {
            name: Ident { symbol: table },
            row: Ident { symbol: row },
        });

        Root {
            stmts: Box::new([struct_stmt, table_stmt]),
        }
    }

    #[test]
    fn named_table_over_struct_is_ok() {
        check(
            |hir, interner| named_table(hir, interner, "Row"),
            expect![""],
        );
    }

    #[test]
    fn inline_table_is_ok() {
        check(
            |hir, interner| {
                let table = interner.intern("T");
                let field = int64_field(hir, interner, "x");
                let table_stmt = hir.alloc_stmt(Stmt::InlineTable {
                    name: Ident { symbol: table },
                    fields: Box::new([field]),
                });
                Root {
                    stmts: Box::new([table_stmt]),
                }
            },
            expect![""],
        );
    }

    // --- infer_stmt: declarations ---

    #[test]
    fn src_struct_registration_ok() {
        check_src("struct P { x: int64 }", expect![""]);
    }

    #[test]
    fn src_named_table_over_struct_ok() {
        check_src("struct R { x: int64 }\ntable T = R", expect![""]);
    }

    #[test]
    fn src_named_table_over_non_struct() {
        check_src("table T = R", expect!["`R` is not a struct"]);
    }

    #[test]
    fn src_inline_table_ok() {
        check_src("table T = { x: int64 }", expect![""]);
    }

    #[test]
    fn src_func_ok() {
        check_src("fn f(x: int64) -> int64 { return x }", expect![""]);
    }

    // --- infer_stmt: let ---

    #[test]
    fn src_let_with_annotation_ok() {
        check_src("let x: int64 = 0", expect![""]);
    }

    #[test]
    fn src_let_widen_coerces() {
        check_src("let a: int32 = 0\nlet b: int64 = a", expect![""]);
    }

    #[test]
    fn src_let_mismatch() {
        check_src(
            "let x: bool = 0",
            expect!["value of type `Int64` is not assignable to `Bool`"],
        );
    }

    #[test]
    fn src_let_without_annotation_ok() {
        check_src("let x = 0", expect![""]);
    }

    // --- infer_stmt: assign ---

    #[test]
    fn src_assign_mutable_var_ok() {
        check_src("let mut x: int64 = 0\nx = 1", expect![""]);
    }

    #[test]
    fn src_assign_immutable_var() {
        check_src(
            "let x: int64 = 0\nx = 1",
            expect!["cannot assign to immutable variable `x`"],
        );
    }

    #[test]
    fn src_assign_value_mismatch() {
        check_src(
            "let mut x: int64 = 0\nx = true",
            expect!["value of type `Bool` is not assignable to `Int64`"],
        );
    }

    #[test]
    fn src_assign_immutable_struct_field() {
        check_src(
            "struct S { f: int64 }\nlet mut s = S { f: 0 }\ns.f = 1",
            expect!["cannot assign to immutable field `f`"],
        );
    }

    #[test]
    fn src_assign_to_non_place() {
        check_src("1 = 2", expect!["cannot assign to this expression"]);
    }

    #[test]
    fn src_assign_field_of_non_place_base() {
        check_src(
            "struct S { x: int64 }\nfn g() -> S { return S { x: 0 } }\ng().x = 1",
            expect![[r#"
                cannot assign to immutable field `x`
                cannot assign to this expression"#]],
        );
    }

    // --- infer_stmt: return ---

    #[test]
    fn src_return_matching_ok() {
        check_src("fn f() -> int64 { return 0 }", expect![""]);
    }

    #[test]
    fn src_return_coerce_widens() {
        check_src(
            "fn f() -> int64 { let a: int32 = 0\nreturn a }",
            expect![""],
        );
    }

    #[test]
    fn src_return_mismatch() {
        check_src(
            "fn f() -> int64 { return true }",
            expect!["value of type `Bool` is not assignable to return type `Int64`"],
        );
    }

    #[test]
    fn src_return_without_value_unit_ok() {
        check_src("fn f() { return }", expect![""]);
    }

    #[test]
    fn src_return_without_value_nonunit() {
        check_src(
            "fn f() -> int64 { return }",
            expect!["value of type `Unit` is not assignable to return type `Int64`"],
        );
    }

    #[test]
    fn src_return_outside_func_ok() {
        check_src("return 0", expect![""]);
    }

    #[test]
    fn src_expr_stmt_ok() {
        check_src("1 + 2", expect![""]);
    }

    // --- infer_expr: literals ---

    #[test]
    fn src_literal_bool_ok() {
        check_src("let a: bool = true", expect![""]);
    }

    #[test]
    fn src_literal_string_ok() {
        check_src("let a: str = \"hi\"", expect![""]);
    }

    #[test]
    fn src_literal_float_ok() {
        check_src("let a: float64 = 0.0", expect![""]);
    }

    // --- infer_expr: ident ---

    #[test]
    fn src_ident_unresolved() {
        check_src("let x = y", expect!["unresolved identifier `y`"]);
    }

    // --- infer_expr: func call ---

    #[test]
    fn src_func_call_ok() {
        check_src(
            "fn f(x: int64) -> int64 { return x }\nlet r: int64 = f(0)",
            expect![""],
        );
    }

    #[test]
    fn src_call_not_callable() {
        check_src(
            "let x: int64 = 0\nlet y = x(1)",
            expect!["type `Int64` is not callable"],
        );
    }

    #[test]
    fn src_call_too_many_args() {
        check_src(
            "fn f(x: int64) -> int64 { return x }\nf(0, 1)",
            expect!["expected 1 argument(s), found 2"],
        );
    }

    #[test]
    fn src_call_too_few_args() {
        check_src(
            "fn f(x: int64) -> int64 { return x }\nf()",
            expect!["expected 1 argument(s), found 0"],
        );
    }

    #[test]
    fn src_call_arg_not_assignable() {
        check_src(
            "fn f(x: bool) -> bool { return x }\nf(0)",
            expect!["argument of type `Int64` is not assignable to parameter of type `Bool`"],
        );
    }

    // --- infer_expr: field access ---

    #[test]
    fn src_field_access_ok() {
        check_src(
            "struct S { x: int64 }\nlet s = S { x: 0 }\nlet v: int64 = s.x",
            expect![""],
        );
    }

    #[test]
    fn src_field_access_unknown_field() {
        check_src(
            "struct S { x: int64 }\nlet s = S { x: 0 }\nlet v = s.y",
            expect!["struct `S` has no field `y`"],
        );
    }

    #[test]
    fn src_field_access_on_non_struct() {
        check_src(
            "let a: int64 = 0\nlet b = a.x",
            expect!["type `Int64` has no fields"],
        );
    }

    // --- infer_expr: struct init ---

    #[test]
    fn src_struct_init_unknown_struct() {
        check_src("let s = M { x: 0 }", expect!["`M` is not a struct"]);
    }

    #[test]
    fn src_struct_init_missing_field() {
        check_src(
            "struct S { x: int64, y: int64 }\nlet s = S { x: 0 }",
            expect!["missing field `y` in `S` literal"],
        );
    }

    #[test]
    fn src_struct_init_unknown_field() {
        check_src(
            "struct S { x: int64 }\nlet s = S { x: 0, y: 1 }",
            expect!["struct `S` has no field `y`"],
        );
    }

    #[test]
    fn src_struct_init_mismatched_field() {
        check_src(
            "struct S { x: bool }\nlet s = S { x: 0 }",
            expect!["value is not assignable to field `x`"],
        );
    }

    #[test]
    fn src_struct_init_duplicate_field() {
        check_src(
            "struct S { x: int64 }\nlet s = S { x: 0, x: 1 }",
            expect!["field `x` is initialized more than once"],
        );
    }

    // --- infer_expr: list init ---

    #[test]
    fn src_list_init_ok() {
        check_src("let l: List[int64] = [1, 2, 3]", expect![""]);
    }

    #[test]
    fn src_list_init_empty_ok() {
        check_src("let l = []", expect![""]);
    }

    // --- coerce: non-numeric source is rejected ---

    #[test]
    fn src_coerce_non_numeric_source_rejected() {
        check_src(
            "let a: bool = true\nlet b: int64 = a",
            expect!["value of type `Bool` is not assignable to `Int64`"],
        );
    }

    // --- infer_rel: queries ---

    const TABLE: &str = "struct Row { a: int64, active: bool }\ntable t = Row\n";

    #[test]
    fn src_select_bare_column_ok() {
        check_src(&format!("{TABLE}let q = from t |> select a"), expect![""]);
    }

    #[test]
    fn src_alias_field_and_where_bool_ok() {
        check_src(
            &format!("{TABLE}let q = from t as e |> where e.active |> select e.a"),
            expect![""],
        );
    }

    #[test]
    fn src_where_non_bool_predicate() {
        check_src(
            &format!("{TABLE}let q = from t |> where a"),
            expect!["`where` predicate must be `bool`, found `Int64`"],
        );
    }

    #[test]
    fn src_from_unknown_table() {
        check_src(
            "let q = from nope |> select x",
            expect!["`nope` is not a table"],
        );
    }

    #[test]
    fn src_select_unknown_column() {
        check_src(
            &format!("{TABLE}let q = from t |> select missing"),
            expect!["unresolved identifier `missing`"],
        );
    }

    #[test]
    fn src_drop_removes_column_from_row() {
        check_src(
            &format!("{TABLE}let q = from t |> drop a |> select a"),
            expect!["unresolved identifier `a`"],
        );
    }

    #[test]
    fn src_rename_then_select_new_name_ok() {
        check_src(
            &format!("{TABLE}let q = from t |> rename a as x |> select x"),
            expect![""],
        );
    }

    #[test]
    fn src_extend_then_select_computed_ok() {
        check_src(
            &format!("{TABLE}let q = from t |> extend a as b |> select b"),
            expect![""],
        );
    }

    #[test]
    fn src_invalid_operator_application_reports() {
        check_src(
            "let x = true + false",
            expect!["operator `+` cannot be applied to `Bool` and `Bool`"],
        );
    }

    #[test]
    fn src_mismatched_membership_reports() {
        check_src(
            "let f = 1.5\nlet xs = [1, 2]\nlet x = f in xs",
            expect!["operator `in` cannot be applied to `Float64` and `List[Int64]`"],
        );
    }

    #[test]
    fn src_distinct_passthrough_ok() {
        check_src(
            &format!("{TABLE}let q = from t |> distinct |> select a"),
            expect![""],
        );
    }

    // --- infer_join_rel ---

    const JOIN_TABLES: &str = "struct Dept { a: int64, name: str }\ntable d = Dept\nstruct Info { code: int64, label: str }\ntable i = Info\nstruct Tag { a: str, note: str }\ntable g = Tag\n";

    #[test]
    fn src_join_on_ok() {
        check_src(
            &format!(
                "{TABLE}{JOIN_TABLES}let q = from t e |> join i x on e.a == x.code |> select e.a, x.label"
            ),
            expect![""],
        );
    }

    #[test]
    fn src_join_on_sees_both_rows_by_bare_name() {
        check_src(
            &format!("{TABLE}{JOIN_TABLES}let q = from t |> join i on a == code |> select label"),
            expect![""],
        );
    }

    #[test]
    fn src_join_on_shared_column_name_is_allowed() {
        check_src(
            &format!("{TABLE}{JOIN_TABLES}let q = from t e |> join d x on e.a == x.a"),
            expect![[r#""#]],
        );
    }

    #[test]
    fn src_qualified_columns_resolve_on_either_side_of_a_join() {
        check_src(
            &format!(
                "{TABLE}{JOIN_TABLES}let q = from t e |> join d x on e.a == x.a |> select e.a as l, x.a as r, x.name"
            ),
            expect![[r#""#]],
        );
    }

    #[test]
    fn src_bare_column_shared_by_both_sides_is_ambiguous() {
        check_src(
            &format!("{TABLE}{JOIN_TABLES}let q = from t e |> join d x on e.a == x.a |> select a"),
            expect!["column `a` is ambiguous; qualify it with a relation alias"],
        );
    }

    #[test]
    fn src_bare_column_from_one_side_still_resolves() {
        check_src(
            &format!(
                "{TABLE}{JOIN_TABLES}let q = from t e |> join d x on e.a == x.a |> select active, name"
            ),
            expect![[r#""#]],
        );
    }

    #[test]
    fn src_chained_joins_resolve_each_qualified_column() {
        check_src(
            &format!(
                "{TABLE}{JOIN_TABLES}let q = from t e |> join d x on e.a == x.a |> join g y on x.name == y.a |> select e.a as ea, x.a as xa, y.note"
            ),
            expect![[r#""#]],
        );
    }

    #[test]
    fn src_join_on_non_bool_condition() {
        check_src(
            &format!("{TABLE}{JOIN_TABLES}let q = from t e |> join i x on e.a"),
            expect!["`on` condition must be `bool`, found `Int64`"],
        );
    }

    #[test]
    fn src_join_using_keeps_one_copy_of_the_key() {
        check_src(
            &format!(
                "{TABLE}{JOIN_TABLES}let q = from t |> join d using (a) |> select a, active, name"
            ),
            expect![""],
        );
    }

    #[test]
    fn src_join_using_unknown_column() {
        check_src(
            &format!("{TABLE}{JOIN_TABLES}let q = from t |> join d using (missing)"),
            expect!["`using` column `missing` is not in the join's left input"],
        );
    }

    #[test]
    fn src_join_using_column_missing_on_the_right() {
        check_src(
            &format!("{TABLE}{JOIN_TABLES}let q = from t |> join i using (a)"),
            expect!["`using` column `a` is not in the joined relation"],
        );
    }

    #[test]
    fn src_join_using_column_types_must_match() {
        check_src(
            &format!("{TABLE}{JOIN_TABLES}let q = from t |> join g using (a)"),
            expect!["`using` column `a` is `Int64` on the left and `String` on the right"],
        );
    }

    #[test]
    fn src_join_using_leaves_a_shared_non_key_column_ambiguous() {
        check_src(
            "struct L { k: int64, v: int64 }\ntable l = L\nstruct R { k: int64, v: int64 }\ntable r = R\nlet q = from l a |> join r b using (k) |> select v",
            expect!["column `v` is ambiguous; qualify it with a relation alias"],
        );
    }

    #[test]
    fn src_join_using_reports_every_bad_column() {
        check_src(
            &format!("{TABLE}{JOIN_TABLES}let q = from t |> join g using (a, active, missing)"),
            expect![[r#"
                `using` column `a` is `Int64` on the left and `String` on the right
                `using` column `active` is not in the joined relation
                `using` column `missing` is not in the join's left input"#]],
        );
    }

    #[test]
    fn src_qualified_rename_names_one_side() {
        check_src(
            &format!(
                "{TABLE}{JOIN_TABLES}let q = from t e |> join d x on e.a == x.a |> rename e.a as ea, x.a as xa |> select ea, xa"
            ),
            expect![[r#""#]],
        );
    }

    #[test]
    fn src_bare_rename_of_a_shared_column_is_ambiguous() {
        check_src(
            &format!(
                "{TABLE}{JOIN_TABLES}let q = from t e |> join d x on e.a == x.a |> rename a as z"
            ),
            expect!["column `a` is ambiguous; qualify it with a relation alias"],
        );
    }

    #[test]
    fn src_qualified_rename_of_an_unknown_column() {
        check_src(
            &format!(
                "{TABLE}{JOIN_TABLES}let q = from t e |> join d x on e.a == x.a |> rename e.nope as z"
            ),
            expect!["`e` has no column `nope` here"],
        );
    }

    #[test]
    fn src_join_unknown_relation() {
        check_src(
            &format!("{TABLE}let q = from t |> join nope on a == 1"),
            expect!["`nope` is not a table"],
        );
    }
}
