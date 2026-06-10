//===----------------------------------------------------------------------===//
// Yuzu ANF (A-normal form) definitions.
//===----------------------------------------------------------------------===//
//
// The ANF tree is the optimization + pre-emit IR, lowered from typed HIR. It
// pairs relational operators (`Rel`) with scalar A-normal let-sequences: every
// non-trivial computation is named by a `LetBind` and its operands are `Atom`s
// (trivial values), so sharing is explicit — a reused bind becomes a projection
// column, a single-use one inlines. Types are known at construction and stored
// on each value-producing node (no re-inference).
//
// Schema (`Variant`, `Node`, `Child`, ...) is shared with the AST/HIR and lives
// in `yuzu/DSL/TreeBase.td`. Consumed by `yuzu-tblgen` with `TreeName = "Anf"`,
// which emits `AnfKind`, the `AnfNode` hierarchy, `AnfBuilder`, and `AnfVisitor`.
//
//===----------------------------------------------------------------------===//

include "yuzu/DSL/TreeBase.td"

//===----------------------------------------------------------------------===//
// Types
//===----------------------------------------------------------------------===//

def Bool : Native<"bool"> { let IsTrivial = 1; }
def Int64 : Native<"int64_t"> { let IsTrivial = 1; }
def Float64 : Native<"double"> { let IsTrivial = 1; }
def String : Native<"std::u32string_view"> { let IsTrivial = 1; }

/// Resolved semantic type carried on every value-producing node. Shared with
/// HIR via `yuzu::types`; ANF is typed by construction.
def TypeRef : Native<"const yuzu::types::Type *"> { let IsTrivial = 1; }

/// Builtin operator descriptor, reused from HIR for now (a later extraction to
/// a shared `yuzu::ops` would mirror the `yuzu::types` move).
def OpRef : Native<"const yuzu::hir::Op *"> { let IsTrivial = 1; }

/// Universal ANF-node id; threaded through every constructor by the generator.
def Id : Native<"yuzu::anf::AnfId"> { let IsTrivial = 1; }

//===----------------------------------------------------------------------===//
// Root of the hierarchy
//===----------------------------------------------------------------------===//

def AnfNode : Base {
    let Namespace = "yuzu::anf";
    let TreeName = "Anf";
    let Fields = (ins Custom<Id>:$id);
}

/// A bare name in a *naming* position — the temp bound by a `LetBind`, a field
/// name, a column alias, a table name. Carries no type: it labels, it is not a
/// value. Use `VarRef` to *read* a binding as a value.
def Ident : Node<AnfNode> {
    let Fields = (ins Custom<String>:$name);
}

//===----------------------------------------------------------------------===//
// Atoms — trivial, already-evaluated values. Each carries its type.
//===----------------------------------------------------------------------===//

def Atom : Variant<AnfNode> {}

/// Reads a binding as a value: an SSA temp bound by a `LetBind`, or a row alias.
def VarRef : Node<Atom> {
    let Fields = (ins
        Child<Ident>:$ident,
        Custom<TypeRef>:$type
    );
}

/// Field / column access `base.field`. Chains for multipart paths
/// (`e.a.b` = `FieldRef(FieldRef(VarRef(e), a), b)`). A selection is trivial, so
/// the base stays an `Atom` — a call result is `LetBind`-bound first.
def FieldRef : Node<Atom> {
    let Fields = (ins
        Child<Atom>:$base,
        Child<Ident>:$field,
        Custom<TypeRef>:$type
    );
}

/// A constant value. Grouped like HIR's `Literal` variant.
def Constant : Variant<Atom> {}

def IntConst : Node<Constant> {
    let Fields = (ins Custom<Int64>:$value, Custom<TypeRef>:$type);
}

def FloatConst : Node<Constant> {
    let Fields = (ins Custom<Float64>:$value, Custom<TypeRef>:$type);
}

def BoolConst : Node<Constant> {
    let Fields = (ins Custom<Bool>:$value, Custom<TypeRef>:$type);
}

def StringConst : Node<Constant> {
    let Fields = (ins Custom<String>:$value, Custom<TypeRef>:$type);
}

//===----------------------------------------------------------------------===//
// Computations — non-trivial expressions over atoms.
//===----------------------------------------------------------------------===//

def Comp : Variant<AnfNode> {}

/// A builtin operator applied to atomic arguments (`a + b`, `-x`). `op` reuses
/// HIR's operator descriptors.
def PrimOp : Node<Comp> {
    let Fields = (ins
        Custom<OpRef>:$op,
        Children<Atom>:$args,
        Custom<TypeRef>:$type
    );
}

//===----------------------------------------------------------------------===//
// Bindings
//===----------------------------------------------------------------------===//

/// `let <ident> = <value>`. Names a computation so it can be referenced (and
/// shared) by `VarRef` — the backbone of a scalar let-sequence.
def LetBind : Node<AnfNode> {
    let Fields = (ins
        Child<Ident>:$ident,
        Child<Comp>:$value
    );
}

//===----------------------------------------------------------------------===//
// Relational operators
//===----------------------------------------------------------------------===//

def Rel : Variant<AnfNode> {}

/// A table scan. `type` is the `Relation[row]` type of the produced rows.
def Scan : Node<Rel> {
    let Fields = (ins
        Child<Ident>:$table,
        Custom<TypeRef>:$type
    );
}

/// One output column of a `Project`: an output name and its value atom.
def Column : Node<AnfNode> {
    let Fields = (ins
        Child<Ident>:$ident,
        Child<Atom>:$value
    );
}

/// A projection over an input relation. `binds` is the scalar let-sequence
/// feeding the columns; a multi-use bind is shared as a column, a single-use
/// one inlines into an output atom.
def Project : Node<Rel> {
    let Fields = (ins
        Child<Rel>:$input,
        Children<LetBind>:$binds,
        Children<Column>:$columns,
        Custom<TypeRef>:$type
    );
}

//===----------------------------------------------------------------------===//
// Root
//===----------------------------------------------------------------------===//

/// Top of an ANF tree: a single relational query plan.
def Root : Node<AnfNode> {
    let Fields = (ins Child<Rel>:$plan);
}
