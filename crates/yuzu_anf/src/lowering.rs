use std::collections::{HashMap, HashSet};

use yuzu_core::adt::{Int, Signedness, StringInterner, SymbolId};
use yuzu_diagnostics::{
    diagnostics::{Span, builder::DiagnosticBuilder, engine::DiagnosticsEngine},
    source_map::SourceId,
};
use yuzu_hir::{self as hir, HirSourceMap, InferenceResult};
use yuzu_types::{Type, TypeCtx, TypeId};

use crate::{
    AnfCtx, AnfSourceMap,
    anf::{
        Atom, AtomId, Binding, BindingId, Const, Expr, ExprId, Ident, JoinCondition, JoinKind, Op,
        Rel, RelId, RenameItem, Root, SelectItem, Stmt, StmtId, StructField, StructFieldInit,
        Thunk,
    },
    symbols::{Symbol, SymbolTable},
};

#[allow(clippy::too_many_arguments)]
pub fn lower(
    root: &hir::Root,
    hir: &hir::HirCtx,
    types: &InferenceResult,
    type_ctx: &TypeCtx,
    anf: &mut AnfCtx,
    interner: &mut StringInterner,
    diagnostics: &mut DiagnosticsEngine,
    hir_source_map: &HirSourceMap,
    source_id: SourceId,
) -> (Root, AnfSourceMap) {
    AnfLowerer {
        anf,
        hir,
        types,
        type_ctx,
        source_map: AnfSourceMap::default(),
        hir_source_map,
        symbols: SymbolTable::new(),
        interner,
        diagnostics,
        source_id,
        pending: Vec::new(),
        temp: 0,
        current_row: None,
        rows: HashSet::new(),
        placements: HashMap::new(),
    }
    .lower_root(root)
}

struct AnfLowerer<'l> {
    anf: &'l mut AnfCtx,
    hir: &'l hir::HirCtx,
    types: &'l InferenceResult,
    type_ctx: &'l TypeCtx,
    symbols: SymbolTable,
    interner: &'l mut StringInterner,
    diagnostics: &'l mut DiagnosticsEngine,
    hir_source_map: &'l HirSourceMap,
    source_map: AnfSourceMap,
    source_id: SourceId,
    // Internal State
    pending: Vec<StmtId>,
    temp: usize,
    current_row: Option<BindingId>,
    /// Every row binding, so `alias.column` is told apart from struct access.
    rows: HashSet<BindingId>,
    /// Where each addressable row's columns sit in the current stage's row.
    placements: HashMap<BindingId, Vec<Option<u32>>>,
}

impl AnfLowerer<'_> {
    fn lower_root(mut self, root: &hir::Root) -> (Root, AnfSourceMap) {
        let stmts = self.lower_stmts(&root.stmts);
        (Root { stmts }, self.source_map)
    }

    fn lower_stmts(&mut self, stmts: &[hir::StmtId]) -> Box<[StmtId]> {
        self.hoist_funcs(stmts);
        let mut out = Vec::new();
        for &id in stmts {
            let lowered = self.lower_stmt(id);
            out.append(&mut self.pending);
            if let Some(lowered) = lowered {
                out.push(lowered);
            }
        }
        out.into_boxed_slice()
    }

    /// Register function names up front so calls can refer to functions declared
    /// later in the same statement list.
    fn hoist_funcs(&mut self, stmts: &[hir::StmtId]) {
        for &id in stmts {
            let symbol = match self.hir.stmt(id) {
                hir::Stmt::Func { name, .. } => name.symbol,
                _ => continue,
            };
            if let Some(ty) = self.types.stmt_ty(id) {
                let binding = self.anf.alloc_binding(Binding {
                    name: Ident { name: symbol },
                    ty,
                });
                self.symbols.bind_func(symbol, binding, ty);
            }
        }
    }

    fn lower_stmt(&mut self, id: hir::StmtId) -> Option<StmtId> {
        let lowered = match self.hir.stmt(id) {
            hir::Stmt::Struct { name, .. } => Some(self.lower_struct_stmt(id, name.symbol)),
            hir::Stmt::Table { name, .. } | hir::Stmt::InlineTable { name, .. } => {
                Some(self.lower_table_stmt(id, name.symbol))
            }
            hir::Stmt::Func {
                name, params, body, ..
            } => Some(self.lower_func_stmt(id, name.symbol, params, *body)),
            hir::Stmt::Let { name, expr, .. } => Some(self.lower_let_stmt(name.symbol, *expr)),
            hir::Stmt::Assign { target, value } => Some(self.lower_assign_stmt(*target, *value)),
            hir::Stmt::Return { expr } => Some(self.lower_return_stmt(*expr)),
            hir::Stmt::Expr { expr } => Some(self.lower_expr_stmt(*expr)),
            hir::Stmt::Block { stmts } => Some(self.lower_block_stmt(stmts)),
            hir::Stmt::Missing => None,
            hir::Stmt::Impl { .. } | hir::Stmt::Trait { .. } => todo!(),
        };
        if let (Some(stmt), Some(&ptr)) = (lowered, self.hir_source_map.stmt(id)) {
            self.source_map.bind_stmt(stmt, ptr);
        }
        lowered
    }

    fn lower_let_stmt(&mut self, name: SymbolId, expr: hir::ExprId) -> StmtId {
        let ty = self.expr_ty(expr);
        let value = self.lower_expr(expr);
        let value = self.emit_expr(expr, value);
        let binding = self.anf.alloc_binding(Binding {
            name: Ident { name },
            ty,
        });
        self.symbols.bind_value(name, binding);
        self.anf.alloc_stmt(Stmt::Let {
            binding,
            expr: value,
        })
    }

    fn lower_struct_stmt(&mut self, id: hir::StmtId, name: SymbolId) -> StmtId {
        let struct_ty = self.decl_ty(id);
        let fields: Box<[StructField]> = match self.type_ctx.ty(struct_ty) {
            Type::Struct(structure) => structure
                .fields
                .iter()
                .map(|&(field, ty)| StructField {
                    name: Ident { name: field },
                    ty,
                })
                .collect(),
            _ => Box::new([]),
        };
        self.anf.alloc_stmt(Stmt::Struct {
            name: Ident { name },
            fields,
        })
    }

    /// Both `table` and `inline table` declarations lower to a table over a row
    /// type; inference has already reduced the row to a struct type either way.
    fn lower_table_stmt(&mut self, id: hir::StmtId, name: SymbolId) -> StmtId {
        let table_ty = self.decl_ty(id);
        let row = match self.type_ctx.ty(table_ty) {
            Type::Relation(relation) => relation.inner,
            _ => table_ty,
        };
        self.anf.alloc_stmt(Stmt::Table {
            name: Ident { name },
            row,
        })
    }

    fn lower_func_stmt(
        &mut self,
        id: hir::StmtId,
        name: SymbolId,
        params: &[hir::FuncParam],
        body: Option<hir::StmtId>,
    ) -> StmtId {
        let binding = match self.symbols.lookup(name) {
            Some(Symbol::Func(binding, _)) => binding,
            _ => unreachable!("a function is hoisted before it is lowered"),
        };
        let func_ty = self.decl_ty(id);
        let (arg_tys, ret) = match self.type_ctx.ty(func_ty) {
            Type::Func(func) => (func.args.clone(), func.ret_type),
            _ => (Vec::new(), func_ty),
        };

        self.symbols.push_scope();
        let bindings: Box<[BindingId]> = params
            .iter()
            .zip(arg_tys)
            .map(|(param, ty)| {
                let binding = self.anf.alloc_binding(Binding {
                    name: Ident {
                        name: param.name.symbol,
                    },
                    ty,
                });
                self.symbols.bind_value(param.name.symbol, binding);
                binding
            })
            .collect();
        let body = body.and_then(|body| self.lower_stmt(body));
        self.symbols.pop_scope();

        self.anf.alloc_stmt(Stmt::Func {
            binding,
            name: Ident { name },
            params: bindings,
            ret,
            body,
        })
    }

    fn lower_assign_stmt(&mut self, target: hir::ExprId, value: hir::ExprId) -> StmtId {
        let place = self.force_atom(target);
        let expr = self.lower_expr(value);
        let value = self.emit_expr(value, expr);
        self.anf.alloc_stmt(Stmt::Assign {
            target: place,
            value,
        })
    }

    fn lower_return_stmt(&mut self, expr: Option<hir::ExprId>) -> StmtId {
        let value = expr.map(|expr| self.force_atom(expr));
        self.anf.alloc_stmt(Stmt::Return { value })
    }

    fn lower_expr_stmt(&mut self, expr: hir::ExprId) -> StmtId {
        let value = self.lower_expr(expr);
        let value = self.emit_expr(expr, value);
        self.anf.alloc_stmt(Stmt::Expr { value })
    }

    fn lower_block_stmt(&mut self, stmts: &[hir::StmtId]) -> StmtId {
        self.symbols.push_scope();
        let stmts = self.lower_stmts(stmts);
        self.symbols.pop_scope();
        self.anf.alloc_stmt(Stmt::Block { stmts })
    }

    fn lower_expr(&mut self, id: hir::ExprId) -> Expr {
        match self.hir.expr(id) {
            hir::Expr::Literal(literal) => self.lower_literal(id, *literal),
            hir::Expr::Ident { value } => self.lower_ident(id, value.symbol),
            hir::Expr::FieldAccess { base, field } => {
                self.lower_field_access(id, *base, field.symbol)
            }
            hir::Expr::Call { op, args } => self.lower_call(id, *op, args),
            hir::Expr::FuncCall { callee, args } => self.lower_func_call(id, *callee, args),
            hir::Expr::MethodCall {
                receiver,
                method,
                args,
            } => self.lower_method_call(id, *receiver, method.symbol, args),
            hir::Expr::StructInit { name, fields } => self.lower_struct_init(id, *name, fields),
            hir::Expr::ListInit { elements } => self.lower_list_init(id, elements),
            hir::Expr::Rel(rel) => {
                self.symbols.push_scope();
                let saved_row = self.current_row.take();
                let rel = self.lower_rel(*rel);
                self.current_row = saved_row;
                self.symbols.pop_scope();
                Expr::Rel(rel)
            }
            hir::Expr::Missing => {
                self.report_expr(id, "expected an expression but found nothing");
                std::process::abort();
            }
        }
    }

    fn lower_literal(&mut self, id: hir::ExprId, literal: hir::Literal) -> Expr {
        let constant = match literal {
            hir::Literal::Bool { value } => Const::Bool { value },
            hir::Literal::Int { value } => Const::Int {
                value: self.retype_int(id, value),
            },
            hir::Literal::Float { value } => Const::Float { value },
            hir::Literal::String { value } => Const::String { value },
            hir::Literal::Missing => {
                self.report_expr(id, "expected a literal found nothing");
                std::process::abort();
            }
        };
        Expr::Atom {
            value: self.anf.intern_atom(Atom::Const(constant)),
        }
    }

    fn lower_ident(&mut self, id: hir::ExprId, name: SymbolId) -> Expr {
        let atom = match self.symbols.lookup(name) {
            Some(Symbol::Value(binding)) => Atom::Var { binding },
            Some(Symbol::Func(binding, ty)) => Atom::FuncRef { binding, ty },
            // A bare column of the current query row.
            None => {
                let row = self
                    .current_row
                    .expect("identifier resolves to a binding or a query column");
                self.column_atom(id, row, name)
            }
        };
        Expr::Atom {
            value: self.anf.intern_atom(atom),
        }
    }

    fn lower_field_access(&mut self, id: hir::ExprId, base: hir::ExprId, field: SymbolId) -> Expr {
        let base = self.force_atom(base);
        let ty = self.expr_ty(id);

        // `alias.column` against a row in scope is a query column; anything else
        // is a field of a compile-time struct.
        if let Atom::Var { binding } = *self.anf.atom(base)
            && self.rows.contains(&binding)
        {
            let atom = self.column_atom(id, binding, field);
            return Expr::Atom {
                value: self.anf.intern_atom(atom),
            };
        }

        Expr::Atom {
            value: self.anf.intern_atom(Atom::Field {
                base,
                field: Ident { name: field },
                ty,
            }),
        }
    }

    /// Resolves `row.name` to its position in the row the current stage's
    /// expressions index.
    fn column_atom(&mut self, id: hir::ExprId, row: BindingId, name: SymbolId) -> Atom {
        let ty = self.expr_ty(id);
        let Some(column) = self.column_of(row, name) else {
            let text = self.interner.text(name).to_string();
            self.report_expr(id, format!("column `{text}` is not available here"));
            return Atom::Column {
                row,
                name: Ident { name },
                column: 0,
                ty,
            };
        };
        Atom::Column {
            row,
            name: Ident { name },
            column,
            ty,
        }
    }

    fn column_of(&self, row: BindingId, name: SymbolId) -> Option<u32> {
        let placement = self.placements.get(&row)?;
        let row_ty = self.anf.binding(row).ty;
        let Type::Struct(fields) = self.type_ctx.ty(row_ty) else {
            return None;
        };
        let index = fields.fields.iter().position(|&(field, _)| field == name)?;
        placement.get(index).copied().flatten()
    }

    /// Places a row's columns at consecutive positions from `start`.
    fn place_row(&mut self, row: BindingId, start: u32, width: u32) {
        self.rows.insert(row);
        self.placements
            .insert(row, (start..start + width).map(Some).collect());
    }

    /// `where`, `extend` and `rename` leave the input's columns where they were,
    /// so every alias stays addressable and only the new row is added.
    fn carry_row(&mut self, ty: TypeId) -> BindingId {
        let row = self.fresh_row(ty);
        self.place_row(row, 0, self.row_width(ty));
        row
    }

    /// The joined relation's columns follow the left input's. A `using` key is
    /// carried once, from the left, so the right's copy of it lands there.
    fn place_joined_row(
        &mut self,
        row: BindingId,
        left_ty: TypeId,
        right_ty: TypeId,
        condition: &hir::JoinCondition,
    ) {
        let keys: Vec<SymbolId> = match condition {
            hir::JoinCondition::On(_) => Vec::new(),
            hir::JoinCondition::Using(columns) => {
                columns.iter().map(|column| column.symbol).collect()
            }
        };

        let left = self.row_fields(left_ty);
        let mut placement = Vec::new();
        let mut next = left.len() as u32;
        for (name, _) in self.row_fields(right_ty) {
            if keys.contains(&name) {
                let carried = left.iter().position(|&(field, _)| field == name);
                placement.push(carried.map(|index| index as u32));
                continue;
            }
            placement.push(Some(next));
            next += 1;
        }

        self.rows.insert(row);
        self.placements.insert(row, placement);
    }

    /// `select` builds an unrelated row, so the aliases that named the old one
    /// stop resolving.
    fn reshape_row(&mut self, ty: TypeId) -> BindingId {
        self.placements.clear();
        self.carry_row(ty)
    }

    /// `drop` shifts every column after the ones it removes.
    fn dropped_row(&mut self, ty: TypeId, input_ty: TypeId, dropped: &[hir::Ident]) -> BindingId {
        let mut moved = Vec::new();
        let mut next = 0;
        for (name, _) in self.row_fields(input_ty) {
            let kept = !dropped.iter().any(|column| column.symbol == name);
            moved.push(kept.then(|| {
                next += 1;
                next - 1
            }));
        }

        for placement in self.placements.values_mut() {
            for column in placement.iter_mut() {
                *column = column.and_then(|index| moved.get(index as usize).copied().flatten());
            }
        }
        self.carry_row(ty)
    }

    fn row_fields(&self, rel_ty: TypeId) -> Vec<(SymbolId, TypeId)> {
        match self.type_ctx.ty(self.rel_row(rel_ty)) {
            Type::Struct(row) => row.fields.clone(),
            _ => Vec::new(),
        }
    }

    fn row_width(&self, rel_ty: TypeId) -> u32 {
        self.row_fields(rel_ty).len() as u32
    }

    fn lower_call(&mut self, id: hir::ExprId, op: hir::Op, args: &[hir::ExprId]) -> Expr {
        Expr::Call {
            op: lower_op(op),
            args: args.iter().map(|&arg| self.force_atom(arg)).collect(),
            ty: self.expr_ty(id),
        }
    }

    fn lower_func_call(
        &mut self,
        id: hir::ExprId,
        callee: hir::ExprId,
        args: &[hir::ExprId],
    ) -> Expr {
        let callee = self.force_atom(callee);
        Expr::FuncCall {
            callee,
            args: args.iter().map(|&arg| self.force_atom(arg)).collect(),
            ty: self.expr_ty(id),
        }
    }

    fn lower_method_call(
        &mut self,
        id: hir::ExprId,
        receiver: hir::ExprId,
        method: SymbolId,
        args: &[hir::ExprId],
    ) -> Expr {
        let receiver = self.force_atom(receiver);
        Expr::MethodCall {
            receiver,
            method: Ident { name: method },
            args: args.iter().map(|&arg| self.force_atom(arg)).collect(),
            ty: self.expr_ty(id),
        }
    }

    fn lower_struct_init(
        &mut self,
        id: hir::ExprId,
        name: SymbolId,
        fields: &[hir::StructFieldInit],
    ) -> Expr {
        let mut inits = Vec::with_capacity(fields.len());
        for field in fields {
            let value = self.force_atom(field.value);
            inits.push(StructFieldInit {
                name: Ident {
                    name: field.name.symbol,
                },
                value,
            });
        }
        Expr::StructInit {
            name,
            fields: inits.into_boxed_slice(),
            ty: self.expr_ty(id),
        }
    }

    fn lower_list_init(&mut self, id: hir::ExprId, elements: &[hir::ExprId]) -> Expr {
        Expr::ListInit {
            ty: self.expr_ty(id),
            elements: elements.iter().map(|&elem| self.force_atom(elem)).collect(),
        }
    }

    fn lower_rel(&mut self, id: hir::RelId) -> RelId {
        let ty = self.rel_ty(id);
        let rel = match self.hir.rel(id).clone() {
            hir::Rel::From { relation, alias } => self.lower_from_rel(relation.symbol, alias, ty),
            hir::Rel::Join {
                left,
                right,
                kind,
                condition,
            } => self.lower_join_rel(left, right, kind, &condition, ty),
            hir::Rel::Select { input, items } => self.lower_select_rel(input, &items, ty),
            hir::Rel::Where { input, predicate } => self.lower_where_rel(input, predicate, ty),
            hir::Rel::Distinct { input } => Rel::Distinct {
                input: self.lower_rel(input),
                ty,
            },
            hir::Rel::Drop { input, items } => self.lower_drop_rel(input, &items, ty),
            hir::Rel::Rename { input, items } => self.lower_rename_rel(input, &items, ty),
            hir::Rel::Extend { input, items } => self.lower_extend_rel(input, &items, ty),
            hir::Rel::Missing => unreachable!("Rel::Missing only exists after a parse error"),
        };
        self.anf.alloc_rel(rel)
    }

    fn lower_from_rel(&mut self, relation: SymbolId, alias: Option<hir::Ident>, ty: TypeId) -> Rel {
        let name = match alias {
            Some(alias) => Ident { name: alias.symbol },
            None => self.fresh_temp(),
        };
        let row_ty = self.rel_row(ty);
        let row = self.make_row(name, row_ty);
        self.place_row(row, 0, self.row_width(ty));
        self.current_row = Some(row);
        if let Some(alias) = alias {
            self.symbols.bind_value(alias.symbol, row);
        }
        Rel::From {
            relation: Ident { name: relation },
            alias: alias.map(|alias| Ident { name: alias.symbol }),
            ty,
        }
    }

    fn lower_join_rel(
        &mut self,
        left: hir::RelId,
        right: hir::RelId,
        kind: hir::JoinKind,
        condition: &hir::JoinCondition,
        ty: TypeId,
    ) -> Rel {
        let left_ty = self.rel_ty(left);
        let left = self.lower_rel(left);
        let right_ty = self.rel_ty(right);
        let right = self.lower_rel(right);
        let right_row = self
            .current_row
            .expect("the joined relation always leaves a row");
        self.place_joined_row(right_row, left_ty, right_ty, condition);

        // `on` sees the joined row, so bare column names reach either side.
        let joined = self.fresh_row(ty);
        self.place_row(joined, 0, self.row_width(ty));
        self.current_row = Some(joined);
        let condition = match condition {
            hir::JoinCondition::On(expr) => JoinCondition::On(self.lower_thunk(*expr)),
            hir::JoinCondition::Using(columns) => JoinCondition::Using(
                columns
                    .iter()
                    .map(|column| Ident {
                        name: column.symbol,
                    })
                    .collect(),
            ),
        };

        Rel::Join {
            left,
            right,
            kind: lower_join_kind(kind),
            condition,
            ty,
        }
    }

    fn lower_select_rel(
        &mut self,
        input: hir::RelId,
        items: &[hir::SelectItem],
        ty: TypeId,
    ) -> Rel {
        let input = self.lower_rel(input);
        let items = items
            .iter()
            .map(|item| self.lower_select_item(item))
            .collect();
        // Hand the next stage this select's output row.
        self.current_row = Some(self.reshape_row(ty));
        Rel::Select { input, items, ty }
    }

    fn lower_where_rel(&mut self, input: hir::RelId, predicate: hir::ExprId, ty: TypeId) -> Rel {
        // `where` keeps the input's row, so `current_row` carries through.
        let input = self.lower_rel(input);
        let predicate = self.lower_thunk(predicate);
        Rel::Where {
            input,
            predicate,
            ty,
        }
    }

    fn lower_drop_rel(&mut self, input: hir::RelId, columns: &[hir::Ident], ty: TypeId) -> Rel {
        let input_ty = self.rel_ty(input);
        let drop_columns = columns;
        let input = self.lower_rel(input);
        let columns = columns
            .iter()
            .map(|column| Ident {
                name: column.symbol,
            })
            .collect();
        self.current_row = Some(self.dropped_row(ty, input_ty, drop_columns));
        Rel::Drop { input, columns, ty }
    }

    fn lower_rename_rel(
        &mut self,
        input: hir::RelId,
        items: &[hir::RenameItem],
        ty: TypeId,
    ) -> Rel {
        let input = self.lower_rel(input);
        let items = items
            .iter()
            .map(|item| RenameItem {
                from: Ident {
                    name: item.from.symbol,
                },
                to: Ident {
                    name: item.to.symbol,
                },
            })
            .collect();
        self.current_row = Some(self.carry_row(ty));
        Rel::Rename { input, items, ty }
    }

    fn lower_extend_rel(
        &mut self,
        input: hir::RelId,
        items: &[hir::SelectItem],
        ty: TypeId,
    ) -> Rel {
        let input = self.lower_rel(input);
        let items = items
            .iter()
            .map(|item| self.lower_select_item(item))
            .collect();
        self.current_row = Some(self.carry_row(ty));
        Rel::Extend { input, items, ty }
    }

    fn lower_select_item(&mut self, item: &hir::SelectItem) -> SelectItem {
        let body = self.lower_thunk(item.expr);
        SelectItem {
            body,
            alias: item.alias.map(|alias| Ident { name: alias.symbol }),
        }
    }

    /// A per-row column/predicate: its temporaries land in a fresh buffer, and
    /// the block resolves to the atom its expression yields.
    fn lower_thunk(&mut self, expr: hir::ExprId) -> Thunk {
        let saved = std::mem::take(&mut self.pending);
        let value = self.force_atom(expr);
        let stmts = std::mem::replace(&mut self.pending, saved);
        Thunk {
            stmts: stmts.into_boxed_slice(),
            value,
        }
    }

    fn make_row(&mut self, name: Ident, row_ty: TypeId) -> BindingId {
        self.anf.alloc_binding(Binding { name, ty: row_ty })
    }

    fn fresh_row(&mut self, rel_ty: TypeId) -> BindingId {
        let name = self.fresh_temp();
        let row_ty = self.rel_row(rel_ty);
        self.make_row(name, row_ty)
    }

    fn rel_ty(&self, id: hir::RelId) -> TypeId {
        self.types
            .rel_ty(id)
            .expect("inference types every relation")
    }

    fn rel_row(&self, relation_ty: TypeId) -> TypeId {
        match self.type_ctx.ty(relation_ty) {
            Type::Relation(relation) => relation.inner,
            _ => relation_ty,
        }
    }

    fn force_atom(&mut self, id: hir::ExprId) -> AtomId {
        match self.lower_expr(id) {
            Expr::Atom { value } => value,
            expr => {
                let ty = self.expr_ty(id);
                let expr = self.emit_expr(id, expr);
                let name = self.fresh_temp();
                let binding = self.anf.alloc_binding(Binding { name, ty });
                let let_stmt = self.anf.alloc_stmt(Stmt::Let { binding, expr });
                if let Some(&ptr) = self.hir_source_map.expr(id) {
                    self.source_map.bind_stmt(let_stmt, ptr);
                }
                self.pending.push(let_stmt);
                self.anf.intern_atom(Atom::Var { binding })
            }
        }
    }

    fn fresh_temp(&mut self) -> Ident {
        let name = self.interner.intern(&format!("%t{}", self.temp));
        self.temp += 1;
        Ident { name }
    }

    fn expr_ty(&self, id: hir::ExprId) -> TypeId {
        self.types
            .expr_ty(id)
            .expect("inference types every expression")
    }

    fn decl_ty(&self, id: hir::StmtId) -> TypeId {
        self.types
            .stmt_ty(id)
            .expect("inference records every declaration type")
    }

    fn retype_int(&self, id: hir::ExprId, value: Int) -> Int {
        let (num_bits, signedness) = match self.type_ctx.ty(self.expr_ty(id)) {
            Type::Int8 => (8, Signedness::Signed),
            Type::Int16 => (16, Signedness::Signed),
            Type::Int32 => (32, Signedness::Signed),
            Type::Int64 => (64, Signedness::Signed),
            Type::UInt8 => (8, Signedness::Unsigned),
            Type::UInt16 => (16, Signedness::Unsigned),
            Type::UInt32 => (32, Signedness::Unsigned),
            Type::UInt64 => (64, Signedness::Unsigned),
            _ => return value,
        };
        value.cast(num_bits, signedness)
    }

    /// Allocate an ANF expression and record where it came from in the source.
    fn emit_expr(&mut self, origin: hir::ExprId, expr: Expr) -> ExprId {
        let id = self.anf.alloc_expr(expr);
        if let Some(&ptr) = self.hir_source_map.expr(origin) {
            self.source_map.bind_expr(id, ptr);
        }
        id
    }

    fn report_expr(&mut self, id: hir::ExprId, message: impl Into<String>) {
        let range = self
            .hir_source_map
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
}

fn lower_join_kind(kind: hir::JoinKind) -> JoinKind {
    match kind {
        hir::JoinKind::Inner => JoinKind::Inner,
        hir::JoinKind::Left => JoinKind::Left,
        hir::JoinKind::Right => JoinKind::Right,
        hir::JoinKind::Full => JoinKind::Full,
    }
}

fn lower_op(op: hir::Op) -> Op {
    match op {
        hir::Op::Add => Op::Add,
        hir::Op::Sub => Op::Sub,
        hir::Op::Mul => Op::Mul,
        hir::Op::Div => Op::Div,
        hir::Op::Pow => Op::Pow,
        hir::Op::And => Op::And,
        hir::Op::Or => Op::Or,
        hir::Op::In => Op::In,
        hir::Op::NotIn => Op::NotIn,
        hir::Op::Eq => Op::Eq,
        hir::Op::Neq => Op::Neq,
        hir::Op::Lt => Op::Lt,
        hir::Op::Lte => Op::Lte,
        hir::Op::Gt => Op::Gt,
        hir::Op::Gte => Op::Gte,
        hir::Op::ShiftLeft => Op::ShiftLeft,
        hir::Op::ShiftRight => Op::ShiftRight,
        hir::Op::UnaryPos => Op::UnaryPos,
        hir::Op::UnaryNeg => Op::UnaryNeg,
        hir::Op::UnaryNot => Op::UnaryNot,
    }
}

#[cfg(test)]
mod tests {
    use expect_test::{Expect, expect};
    use yuzu_ast::ast::{AstNode, Root as AstRoot};
    use yuzu_core::adt::StringInterner;
    use yuzu_diagnostics::{diagnostics::engine::DiagnosticsEngine, source_map::SourceMap};
    use yuzu_hir::HirCtx;
    use yuzu_lexer::lexer::{Lexer, Token};
    use yuzu_types::TypeCtx;

    use crate::{AnfCtx, dump, lower};

    const TABLE: &str = "struct Row { a: int32, b: int32, active: bool }\ntable t = Row\n";

    fn check(input: &str, expected: Expect) {
        let mut interner = StringInterner::new();
        let mut diagnostics = DiagnosticsEngine::new();
        let mut sources = SourceMap::new();
        let source_id = sources.add("test".to_string(), input.to_string());

        let tokens: Vec<Token> = Lexer::new(input).collect();
        let syntax = yuzu_parser::parse(&tokens, &mut diagnostics, source_id);
        let ast_root = AstRoot::cast(syntax).expect("root node");

        let mut hir = HirCtx::new();
        let (hir_root, hir_source_map) = yuzu_hir::lower(
            ast_root,
            &mut hir,
            &mut interner,
            &mut diagnostics,
            source_id,
        );

        let mut types = TypeCtx::new();
        let inference = yuzu_hir::infer(
            &hir_root,
            &hir,
            &mut interner,
            &mut types,
            &mut diagnostics,
            &hir_source_map,
            source_id,
        );

        let messages: Vec<&str> = diagnostics
            .diagnostics()
            .iter()
            .map(|d| d.message.as_str())
            .collect();
        assert!(
            messages.is_empty(),
            "program should type-check cleanly, got: {messages:?}"
        );

        let mut anf = AnfCtx::new();
        let (anf_root, _anf_source_map) = lower(
            &hir_root,
            &hir,
            &inference,
            &types,
            &mut anf,
            &mut interner,
            &mut diagnostics,
            &hir_source_map,
            source_id,
        );

        expected.assert_eq(&dump(&anf, &interner, &anf_root));
    }

    /// Snapshots the diagnostics lowering itself reports, for programs that
    /// type-check but name a column no stage can supply.
    fn check_error(input: &str, expected: Expect) {
        let mut interner = StringInterner::new();
        let mut diagnostics = DiagnosticsEngine::new();
        let mut sources = SourceMap::new();
        let source_id = sources.add("test".to_string(), input.to_string());

        let tokens: Vec<Token> = Lexer::new(input).collect();
        let syntax = yuzu_parser::parse(&tokens, &mut diagnostics, source_id);
        let ast_root = AstRoot::cast(syntax).expect("root node");

        let mut hir = HirCtx::new();
        let (hir_root, hir_source_map) = yuzu_hir::lower(
            ast_root,
            &mut hir,
            &mut interner,
            &mut diagnostics,
            source_id,
        );

        let mut types = TypeCtx::new();
        let inference = yuzu_hir::infer(
            &hir_root,
            &hir,
            &mut interner,
            &mut types,
            &mut diagnostics,
            &hir_source_map,
            source_id,
        );

        let mut anf = AnfCtx::new();
        let before = diagnostics.diagnostics().len();
        lower(
            &hir_root,
            &hir,
            &inference,
            &types,
            &mut anf,
            &mut interner,
            &mut diagnostics,
            &hir_source_map,
            source_id,
        );

        let reported: Vec<String> = diagnostics
            .diagnostics()
            .iter()
            .skip(before)
            .map(|d| d.message.clone())
            .collect();
        expected.assert_eq(&reported.join("\n"));
    }

    #[test]
    fn binary_expr_flattens_into_temps() {
        check(
            "let a = 1 + 2 * 3",
            expect![[r#"
                let %t0 = mul(2i64, 3i64)
                let a = add(1i64, %t0)
            "#]],
        );
    }

    #[test]
    fn func_call_references_function_and_arg() {
        check(
            "fn add_one(x: int32) -> int32 { return x + 1 }\nlet a = add_one(7)",
            expect![[r#"
                fn add_one(x) {
                  let %t0 = add(x, 1i32)
                  return %t0
                }
                let a = add_one(7i32)
            "#]],
        );
    }

    // ---- Statements ------------------------------------------------------

    #[test]
    fn let_binds_simple_literal() {
        check(
            "let a = 5",
            expect![[r#"
                let a = 5i64
            "#]],
        );
    }

    #[test]
    fn let_mut_lowers_like_let() {
        check(
            "let mut a = 5",
            expect![[r#"
                let a = 5i64
            "#]],
        );
    }

    #[test]
    fn assign_to_local_variable() {
        check(
            "let mut a = 1\na = 2",
            expect![[r#"
                let a = 1i64
                a = 2i64
            "#]],
        );
    }

    #[test]
    fn return_with_value_in_func() {
        check(
            "fn f(x: int32) -> int32 { return x }",
            expect![[r#"
                fn f(x) {
                  return x
                }
            "#]],
        );
    }

    #[test]
    fn bare_return_in_func() {
        check(
            "fn f() { return }",
            expect![[r#"
            fn f() {
              return
            }
        "#]],
        );
    }

    #[test]
    fn expr_statement_calls_function() {
        check(
            "fn f(x: int32) -> int32 { return x }\nf(3)",
            expect![[r#"
                fn f(x) {
                  return x
                }
                f(3i32)
            "#]],
        );
    }

    #[test]
    fn struct_declaration() {
        check(
            "struct P { x: int32 }",
            expect![[r#"
            struct P { x }
        "#]],
        );
    }

    #[test]
    fn struct_declaration_with_multiple_fields() {
        check(
            "struct P { x: int32, y: int32 }",
            expect![[r#"
            struct P { x, y }
        "#]],
        );
    }

    #[test]
    fn named_table_over_struct() {
        check(
            "struct Row { x: int32 }\ntable T = Row",
            expect![[r#"
                struct Row { x }
                table T
            "#]],
        );
    }

    #[test]
    fn inline_table_declaration() {
        check(
            "table T = { x: int32 }",
            expect![[r#"
            table T
        "#]],
        );
    }

    #[test]
    fn func_with_multiple_params_body_and_return() {
        check(
            "fn add(x: int32, y: int32) -> int32 { return x + y }",
            expect![[r#"
                fn add(x, y) {
                  let %t0 = add(x, y)
                  return %t0
                }
            "#]],
        );
    }

    // ---- Binary operators: arithmetic (numeric operands) -----------------

    #[test]
    fn op_add() {
        check(
            "let a = 1 + 2",
            expect![[r#"
                let a = add(1i64, 2i64)
            "#]],
        );
    }

    #[test]
    fn op_sub() {
        check(
            "let a = 3 - 1",
            expect![[r#"
                let a = sub(3i64, 1i64)
            "#]],
        );
    }

    #[test]
    fn op_mul() {
        check(
            "let a = 2 * 3",
            expect![[r#"
                let a = mul(2i64, 3i64)
            "#]],
        );
    }

    #[test]
    fn op_div() {
        check(
            "let a = 6 / 2",
            expect![[r#"
                let a = div(6i64, 2i64)
            "#]],
        );
    }

    #[test]
    fn op_pow() {
        check(
            "let a = 2 ** 3",
            expect![[r#"
                let a = pow(2i64, 3i64)
            "#]],
        );
    }

    #[test]
    fn op_shift_left() {
        check(
            "let a = 1 << 2",
            expect![[r#"
                let a = shl(1i64, 2i64)
            "#]],
        );
    }

    #[test]
    fn op_shift_right() {
        check(
            "let a = 8 >> 1",
            expect![[r#"
                let a = shr(8i64, 1i64)
            "#]],
        );
    }

    // ---- Binary operators: comparison (bool result) ----------------------

    #[test]
    fn op_eq() {
        check(
            "let a = 1 == 2",
            expect![[r#"
                let a = eq(1i64, 2i64)
            "#]],
        );
    }

    #[test]
    fn op_neq() {
        check(
            "let a = 1 != 2",
            expect![[r#"
                let a = neq(1i64, 2i64)
            "#]],
        );
    }

    #[test]
    fn op_lt() {
        check(
            "let a = 1 < 2",
            expect![[r#"
                let a = lt(1i64, 2i64)
            "#]],
        );
    }

    #[test]
    fn op_lte() {
        check(
            "let a = 1 <= 2",
            expect![[r#"
                let a = lte(1i64, 2i64)
            "#]],
        );
    }

    #[test]
    fn op_gt() {
        check(
            "let a = 1 > 2",
            expect![[r#"
                let a = gt(1i64, 2i64)
            "#]],
        );
    }

    #[test]
    fn op_gte() {
        check(
            "let a = 1 >= 2",
            expect![[r#"
                let a = gte(1i64, 2i64)
            "#]],
        );
    }

    // ---- Binary operators: logical (bool operands) -----------------------

    #[test]
    fn op_and() {
        check(
            "let a = true and false",
            expect![[r#"
            let a = and(true, false)
        "#]],
        );
    }

    #[test]
    fn op_or() {
        check(
            "let a = true or false",
            expect![[r#"
            let a = or(true, false)
        "#]],
        );
    }

    // ---- Binary operators: membership (string operands) ------------------

    #[test]
    fn op_in() {
        check(
            r#"let a = "x" in "y""#,
            expect![[r#"
            let a = in("x", "y")
        "#]],
        );
    }

    #[test]
    fn op_not_in() {
        check(
            r#"let a = "x" not in "y""#,
            expect![[r#"
            let a = not_in("x", "y")
        "#]],
        );
    }

    // ---- Unary operators -------------------------------------------------

    #[test]
    fn unary_pos() {
        check(
            "let a = +5",
            expect![[r#"
                let a = pos(5i64)
            "#]],
        );
    }

    #[test]
    fn unary_neg() {
        check(
            "let a = -5",
            expect![[r#"
                let a = neg(5i64)
            "#]],
        );
    }

    #[test]
    fn unary_not() {
        check(
            "let a = not true",
            expect![[r#"
            let a = not(true)
        "#]],
        );
    }

    // ---- Calls, struct/list init, field access ---------------------------

    #[test]
    fn func_call_with_multiple_args() {
        check(
            "fn add(x: int32, y: int32) -> int32 { return x + y }\nlet a = add(1, 2)",
            expect![[r#"
                fn add(x, y) {
                  let %t0 = add(x, y)
                  return %t0
                }
                let a = add(1i32, 2i32)
            "#]],
        );
    }

    #[test]
    fn func_name_resolves_to_funcref() {
        check(
            "fn f(x: int32) -> int32 { return x }\nlet a = f(1)",
            expect![[r#"
                fn f(x) {
                  return x
                }
                let a = f(1i32)
            "#]],
        );
    }

    #[test]
    fn struct_init_lowers_fields() {
        check(
            "struct P { x: int32 }\nlet p = P { x: 1 }",
            expect![[r#"
                struct P { x }
                let p = P { x: 1i32 }
            "#]],
        );
    }

    #[test]
    fn list_init_lowers_elements() {
        check(
            "let xs = [1, 2, 3]",
            expect![[r#"
                let xs = [1i64, 2i64, 3i64]
            "#]],
        );
    }

    #[test]
    fn field_access_reads_field() {
        check(
            "struct P { x: int32 }\nlet p = P { x: 1 }\nlet y = p.x",
            expect![[r#"
                struct P { x }
                let p = P { x: 1i32 }
                let y = p.x
            "#]],
        );
    }

    // ---- Literals --------------------------------------------------------

    #[test]
    fn literal_int() {
        check(
            "let a = 42",
            expect![[r#"
                let a = 42i64
            "#]],
        );
    }

    #[test]
    fn literal_float() {
        check(
            "let a = 3.14",
            expect![[r#"
            let a = 3.14f64
        "#]],
        );
    }

    #[test]
    fn literal_bool() {
        check(
            "let a = true",
            expect![[r#"
            let a = true
        "#]],
        );
    }

    #[test]
    fn literal_string() {
        check(
            r#"let a = "hello""#,
            expect![[r#"
            let a = "hello"
        "#]],
        );
    }

    // ---- Flattening, sharing, scoping ------------------------------------

    #[test]
    fn deep_nesting_creates_ordered_temps() {
        check(
            "let a = 1 + 2 * 3 - 4",
            expect![[r#"
                let %t0 = mul(2i64, 3i64)
                let %t1 = add(1i64, %t0)
                let a = sub(%t1, 4i64)
            "#]],
        );
    }

    #[test]
    fn shared_variable_reuses_atom() {
        check(
            "let x = 5\nlet a = x + x",
            expect![[r#"
                let x = 5i64
                let a = add(x, x)
            "#]],
        );
    }

    #[test]
    fn local_ident_resolves_to_var() {
        check(
            "let x = 1\nlet y = x",
            expect![[r#"
                let x = 1i64
                let y = x
            "#]],
        );
    }

    #[test]
    fn param_shadows_outer_binding() {
        check(
            "let x = 1\nfn f(x: int32) -> int32 { return x }",
            expect![[r#"
                let x = 1i64
                fn f(x) {
                  return x
                }
            "#]],
        );
    }

    #[test]
    fn query_alias_field_and_bare_column() {
        check(
            &format!("{TABLE}let q = from t as e |> where e.active |> select e.a, b"),
            expect![[r#"
                struct Row { a, b, active }
                table t
                let q = from t as e
                  |> where e.active
                  |> select e.a, e.b
            "#]],
        );
    }

    #[test]
    fn query_compound_column_becomes_thunk() {
        check(
            &format!("{TABLE}let q = from t |> select a + b as sum"),
            expect![[r#"
                struct Row { a, b, active }
                table t
                let q = from t
                  |> select add(%t0.a, %t0.b) as sum
            "#]],
        );
    }

    const JOIN_TABLES: &str = "struct Dept { code: int32, name: str }\ntable d = Dept\nstruct Other { a: int32, c: int32 }\ntable u = Other\n";

    #[test]
    fn reports_an_alias_that_outlived_its_row() {
        check_error(
            &format!("{TABLE}from t e |> select a |> where e.b > 0"),
            expect!["column `b` is not available here"],
        );
    }

    #[test]
    fn query_join_on_resolves_both_sides() {
        check(
            &format!("{TABLE}{JOIN_TABLES}let q = from t as e |> join d as x on e.a == x.code"),
            expect![[r#"
                struct Row { a, b, active }
                table t
                struct Dept { code, name }
                table d
                struct Other { a, c }
                table u
                let q = from t as e
                  |> inner join from d as x on eq(e.a, x.code)
            "#]],
        );
    }

    #[test]
    fn query_join_on_bare_column_uses_the_joined_row() {
        check(
            &format!("{TABLE}{JOIN_TABLES}let q = from t |> join d on a == code"),
            expect![[r#"
                struct Row { a, b, active }
                table t
                struct Dept { code, name }
                table d
                struct Other { a, c }
                table u
                let q = from t
                  |> inner join from d on eq(%t2.a, %t2.code)
            "#]],
        );
    }

    #[test]
    fn query_join_using_keeps_its_columns() {
        check(
            &format!("{TABLE}{JOIN_TABLES}let q = from t |> left join u using (a)"),
            expect![[r#"
                struct Row { a, b, active }
                table t
                struct Dept { code, name }
                table d
                struct Other { a, c }
                table u
                let q = from t
                  |> left join from u using a
            "#]],
        );
    }

    #[test]
    fn query_drop_rename_extend_distinct() {
        check(
            &format!(
                "{TABLE}let q = from t |> drop b |> rename a as x |> extend active as flag |> distinct"
            ),
            expect![[r#"
                struct Row { a, b, active }
                table t
                let q = from t
                  |> drop b
                  |> rename a as x
                  |> extend %t2.active as flag
                  |> distinct
            "#]],
        );
    }
}
