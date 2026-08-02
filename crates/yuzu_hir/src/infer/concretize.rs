use yuzu_types::{SymbolId, Type, TypeId};

use crate::{ExprId, RelId, infer::InferCtx};

impl InferCtx<'_> {
    pub(crate) fn concretize(&mut self) {
        let expr_ids: Vec<ExprId> = self.result.expr_types.keys().copied().collect();
        for id in expr_ids {
            let ty = self.result.expr_types[&id];
            let concretized = self.concretize_ty(ty);
            self.result.expr_types.insert(id, concretized);
        }

        let rel_ids: Vec<RelId> = self.result.rel_types.keys().copied().collect();
        for id in rel_ids {
            let ty = self.result.rel_types[&id];
            let concretized = self.concretize_ty(ty);
            self.result.rel_types.insert(id, concretized);
        }
    }

    fn concretize_ty(&mut self, ty: TypeId) -> TypeId {
        let resolved_ty = self.resolve(ty);
        match self.types.ty(resolved_ty).clone() {
            Type::List(l) => {
                let inner = self.concretize_ty(l.inner);
                self.types.list_ty(inner)
            }
            Type::Relation(r) => {
                let columns = r
                    .columns
                    .clone()
                    .into_iter()
                    .map(|column| yuzu_types::Column {
                        ty: self.concretize_ty(column.ty),
                        ..column
                    })
                    .collect();
                self.types.relation_ty(columns)
            }
            Type::Func(f) => {
                let args: Vec<TypeId> = f.args.iter().map(|&a| self.concretize_ty(a)).collect();
                let ret = self.concretize_ty(f.ret_type);
                self.types.func_ty(args, ret)
            }
            Type::Struct(s) => {
                let fields: Vec<(SymbolId, TypeId)> = s
                    .fields
                    .iter()
                    .map(|&(n, t)| (n, self.concretize_ty(t)))
                    .collect();
                self.types.struct_ty(s.name, fields)
            }
            _ => resolved_ty,
        }
    }
}
