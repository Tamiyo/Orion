use yuzu_syntax::{SyntaxElement, SyntaxKind, SyntaxNode, SyntaxToken};

pub trait AstNode: Sized {
    fn can_cast(kind: SyntaxKind) -> bool;
    fn cast(node: SyntaxNode) -> Option<Self>;
    fn syntax(&self) -> &SyntaxNode;
}

macro_rules! ast_node {
    ($name:ident) => {
        #[derive(Debug, Clone, PartialEq, Eq, Hash)]
        pub struct $name(SyntaxNode);

        impl AstNode for $name {
            fn can_cast(kind: SyntaxKind) -> bool {
                kind == SyntaxKind::$name
            }

            fn cast(node: SyntaxNode) -> Option<Self> {
                Self::can_cast(node.kind()).then(|| Self(node))
            }

            fn syntax(&self) -> &SyntaxNode {
                &self.0
            }
        }
    };
}

macro_rules! ast_enum {
    ($name:ident, { $($variant:ident),+ $(,)? }) => {
        #[derive(Debug, Clone, PartialEq, Eq, Hash)]
        pub enum $name {
            $($variant($variant)),+
        }

        impl AstNode for $name {
            fn can_cast(kind: SyntaxKind) -> bool {
                $($variant::can_cast(kind))||+
            }

            fn cast(node: SyntaxNode) -> Option<Self> {
                $(
                    if $variant::can_cast(node.kind()) {
                        return $variant::cast(node).map(Self::$variant);
                    }
                )+
                None
            }

            fn syntax(&self) -> &SyntaxNode {
                match self {
                    $(Self::$variant(it) => it.syntax()),+
                }
            }
        }
    };
}

mod support {
    use super::AstNode;
    use yuzu_syntax::SyntaxNode;

    pub(super) fn child<N: AstNode>(parent: &SyntaxNode) -> Option<N> {
        parent.children().find_map(N::cast)
    }

    pub(super) fn nth_child<N: AstNode>(parent: &SyntaxNode, n: usize) -> Option<N> {
        parent.children().filter_map(N::cast).nth(n)
    }

    pub(super) fn children<'a, N: AstNode + 'a>(
        parent: &'a SyntaxNode,
    ) -> impl Iterator<Item = N> + 'a {
        parent.children().filter_map(N::cast)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum Mutability {
    Mutable,
    Immutable,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum Visibility {
    Public,
    Protected,
    Internal,
    Private,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum BinOp {
    And,
    Or,
    In,
    NotIn,
    Add,
    Sub,
    Mul,
    Div,
    Pow,
    Eq,
    Neq,
    Lt,
    Lte,
    Gt,
    Gte,
    ShiftLeft,
    ShiftRight,
}

impl BinOp {
    fn from_token(token: SyntaxToken) -> Option<Self> {
        Some(match token.kind() {
            SyntaxKind::Plus => BinOp::Add,
            SyntaxKind::Minus => BinOp::Sub,
            SyntaxKind::Star => BinOp::Mul,
            SyntaxKind::Slash => BinOp::Div,
            SyntaxKind::StarStar => BinOp::Pow,
            SyntaxKind::EqEq => BinOp::Eq,
            SyntaxKind::Neq => BinOp::Neq,
            SyntaxKind::Lt => BinOp::Lt,
            SyntaxKind::Lte => BinOp::Lte,
            SyntaxKind::Gt => BinOp::Gt,
            SyntaxKind::Gte => BinOp::Gte,
            SyntaxKind::Shl => BinOp::ShiftLeft,
            SyntaxKind::Shr => BinOp::ShiftRight,
            SyntaxKind::AndKw => BinOp::And,
            SyntaxKind::OrKw => BinOp::Or,
            SyntaxKind::InKw => BinOp::In,
            SyntaxKind::NotKw => BinOp::NotIn,
            _ => return None,
        })
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum UnaryOp {
    Neg,
    Pos,
    Not,
}

impl UnaryOp {
    fn from_token(token: SyntaxToken) -> Option<Self> {
        Some(match token.kind() {
            SyntaxKind::Plus => UnaryOp::Pos,
            SyntaxKind::Minus => UnaryOp::Neg,
            SyntaxKind::NotKw => UnaryOp::Not,
            _ => return None,
        })
    }
}

ast_node!(Root);
impl Root {
    pub fn stmts(&self) -> impl Iterator<Item = Stmt> + '_ {
        support::children(self.syntax())
    }
}

ast_node!(Ident);
impl Ident {
    pub fn text(&self) -> Option<String> {
        Some(self.0.first_token()?.text().to_string())
    }
}

ast_enum!(TypeAnnotation, {
    NamedTypeAnnotation,
    FuncTypeAnnotation,
});

ast_node!(NamedTypeAnnotation);
impl NamedTypeAnnotation {
    pub fn name(&self) -> Option<Ident> {
        support::child(self.syntax())
    }
    pub fn args(&self) -> impl Iterator<Item = TypeAnnotation> + '_ {
        support::children(self.syntax())
    }
}

ast_node!(FuncTypeAnnotation);
impl FuncTypeAnnotation {
    pub fn params(&self) -> Option<FuncTypeAnnotationParams> {
        support::child(self.syntax())
    }
    pub fn result(&self) -> Option<TypeAnnotation> {
        support::child(self.syntax())
    }
}

ast_node!(FuncTypeAnnotationParams);
impl FuncTypeAnnotationParams {
    pub fn params(&self) -> impl Iterator<Item = TypeAnnotation> + '_ {
        support::children(self.syntax())
    }
}

ast_node!(TypeParam);
impl TypeParam {
    pub fn name(&self) -> Option<Ident> {
        support::child(self.syntax())
    }
}

ast_node!(TypeBound);
impl TypeBound {
    pub fn subject(&self) -> Option<Ident> {
        support::child(self.syntax())
    }

    pub fn traits(&self) -> impl Iterator<Item = TraitRef> + '_ {
        support::children(self.syntax())
    }
}

ast_node!(TraitRef);
impl TraitRef {
    pub fn name(&self) -> Option<Ident> {
        support::child(self.syntax())
    }
}

ast_enum!(Stmt, {
    StructStmt,
    TraitStmt,
    ImplStmt,
    FuncStmt,
    TableStmt,
    BlockStmt,
    LetStmt,
    AssignStmt,
    ReturnStmt,
    ExprStmt,
});

ast_node!(StructStmt);
impl StructStmt {
    pub fn name(&self) -> Option<Ident> {
        support::child(self.syntax())
    }

    pub fn fields(&self) -> impl Iterator<Item = StructField> + '_ {
        support::children(self.syntax())
    }
}

ast_node!(StructField);
impl StructField {
    pub fn name(&self) -> Option<Ident> {
        support::child(self.syntax())
    }

    pub fn mutability(&self) -> Mutability {
        let is_mut = self
            .syntax()
            .children_with_tokens()
            .filter_map(SyntaxElement::into_token)
            .any(|token| token.kind() == SyntaxKind::MutKw);

        if is_mut {
            Mutability::Mutable
        } else {
            Mutability::Immutable
        }
    }

    pub fn ty(&self) -> Option<TypeAnnotation> {
        support::child(self.syntax())
    }
}

ast_node!(TraitStmt);
impl TraitStmt {
    pub fn name(&self) -> Option<Ident> {
        support::child(self.syntax())
    }

    pub fn methods(&self) -> impl Iterator<Item = FuncStmt> + '_ {
        support::children(self.syntax())
    }
}

ast_node!(ImplStmt);
impl ImplStmt {
    pub fn trait_(&self) -> Option<TraitRef> {
        support::child(self.syntax())
    }

    pub fn ty(&self) -> Option<Ident> {
        support::child(self.syntax())
    }

    pub fn methods(&self) -> impl Iterator<Item = FuncStmt> + '_ {
        support::children(self.syntax())
    }
}

ast_node!(FuncStmt);
impl FuncStmt {
    pub fn name(&self) -> Option<Ident> {
        support::child(self.syntax())
    }

    pub fn type_params(&self) -> impl Iterator<Item = TypeParam> + '_ {
        support::children(self.syntax())
    }

    pub fn params(&self) -> impl Iterator<Item = FuncParam> + '_ {
        support::children(self.syntax())
    }

    pub fn result(&self) -> Option<TypeAnnotation> {
        support::child(self.syntax())
    }

    pub fn bounds(&self) -> impl Iterator<Item = TypeBound> + '_ {
        support::children(self.syntax())
    }

    pub fn body(&self) -> Option<BlockStmt> {
        support::child(self.syntax())
    }
}

ast_node!(FuncParam);
impl FuncParam {
    pub fn name(&self) -> Option<Ident> {
        support::child(self.syntax())
    }

    pub fn ty(&self) -> Option<TypeAnnotation> {
        support::child(self.syntax())
    }
}

ast_node!(TableStmt);
impl TableStmt {
    pub fn name(&self) -> Option<Ident> {
        support::nth_child(self.syntax(), 0)
    }

    pub fn row_struct(&self) -> Option<Ident> {
        support::nth_child(self.syntax(), 1)
    }

    pub fn inline_fields(&self) -> impl Iterator<Item = StructField> + '_ {
        support::children(self.syntax())
    }
}

ast_node!(BlockStmt);
impl BlockStmt {
    pub fn stmts(&self) -> impl Iterator<Item = Stmt> + '_ {
        support::children(self.syntax())
    }
}

ast_node!(LetStmt);
impl LetStmt {
    pub fn name(&self) -> Option<Ident> {
        support::child(self.syntax())
    }

    pub fn mutability(&self) -> Mutability {
        let is_mut = self
            .syntax()
            .children_with_tokens()
            .filter_map(SyntaxElement::into_token)
            .any(|token| token.kind() == SyntaxKind::MutKw);

        if is_mut {
            Mutability::Mutable
        } else {
            Mutability::Immutable
        }
    }

    pub fn type_annotation(&self) -> Option<TypeAnnotation> {
        support::child(self.syntax())
    }

    pub fn expr(&self) -> Option<Expr> {
        support::child(self.syntax())
    }
}

ast_node!(AssignStmt);
impl AssignStmt {
    pub fn target(&self) -> Option<Expr> {
        support::nth_child(self.syntax(), 0)
    }

    pub fn value(&self) -> Option<Expr> {
        support::nth_child(self.syntax(), 1)
    }
}

ast_node!(ReturnStmt);
impl ReturnStmt {
    pub fn expr(&self) -> Option<Expr> {
        support::child(self.syntax())
    }
}

ast_node!(ExprStmt);
impl ExprStmt {
    pub fn expr(&self) -> Option<Expr> {
        support::child(self.syntax())
    }
}

ast_enum!(Expr, {
    IdentExpr,
    CallExpr,
    FieldAccessExpr,
    StructExpr,
    ListExpr,
    BinaryExpr,
    UnaryExpr,
    ParenExpr,
    Literal,
    Rel,
});

ast_node!(IdentExpr);
impl IdentExpr {
    pub fn name(&self) -> Option<Ident> {
        support::child(self.syntax())
    }
}

ast_node!(CallExpr);
impl CallExpr {
    pub fn callee(&self) -> Option<Expr> {
        support::child(self.syntax())
    }

    pub fn args(&self) -> Option<ArgList> {
        support::child(self.syntax())
    }
}

ast_node!(ArgList);
impl ArgList {
    pub fn args(&self) -> impl Iterator<Item = Expr> + '_ {
        support::children(self.syntax())
    }
}

ast_node!(FieldAccessExpr);
impl FieldAccessExpr {
    pub fn base(&self) -> Option<Expr> {
        support::child(self.syntax())
    }

    pub fn field(&self) -> Option<Ident> {
        support::child(self.syntax())
    }
}

ast_node!(StructExpr);
impl StructExpr {
    pub fn name(&self) -> Option<Ident> {
        support::child(self.syntax())
    }

    pub fn fields(&self) -> impl Iterator<Item = StructFieldInit> + '_ {
        support::children(self.syntax())
    }
}

ast_node!(StructFieldInit);
impl StructFieldInit {
    pub fn name(&self) -> Option<Ident> {
        support::child(self.syntax())
    }

    pub fn value(&self) -> Option<Expr> {
        support::child(self.syntax())
    }
}

ast_node!(ListExpr);
impl ListExpr {
    pub fn elements(&self) -> impl Iterator<Item = Expr> + '_ {
        support::children(self.syntax())
    }
}

ast_node!(BinaryExpr);
impl BinaryExpr {
    pub fn lhs(&self) -> Option<Expr> {
        support::nth_child(self.syntax(), 0)
    }

    pub fn op(&self) -> Option<BinOp> {
        self.syntax()
            .children_with_tokens()
            .filter_map(SyntaxElement::into_token)
            .find_map(BinOp::from_token)
    }

    pub fn rhs(&self) -> Option<Expr> {
        support::nth_child(self.syntax(), 1)
    }
}

ast_node!(UnaryExpr);
impl UnaryExpr {
    pub fn op(&self) -> Option<UnaryOp> {
        self.syntax()
            .children_with_tokens()
            .filter_map(SyntaxElement::into_token)
            .find_map(UnaryOp::from_token)
    }

    pub fn expr(&self) -> Option<Expr> {
        support::child(self.syntax())
    }
}

ast_node!(ParenExpr);
impl ParenExpr {
    pub fn expr(&self) -> Option<Expr> {
        support::child(self.syntax())
    }
}

ast_enum!(Rel, {
    FromExpr,
    SelectExpr,
    WhereExpr,
    DistinctExpr,
    DropExpr,
    RenameExpr,
    ExtendExpr,
});

ast_node!(FromExpr);
impl FromExpr {
    pub fn relation(&self) -> Option<Ident> {
        support::nth_child(self.syntax(), 0)
    }

    pub fn alias(&self) -> Option<Ident> {
        support::nth_child(self.syntax(), 1)
    }
}

ast_node!(SelectExpr);
impl SelectExpr {
    pub fn input(&self) -> Option<Expr> {
        support::child(self.syntax())
    }

    pub fn items(&self) -> impl Iterator<Item = SelectItem> + '_ {
        support::children(self.syntax())
    }
}

ast_node!(SelectItem);
impl SelectItem {
    pub fn expr(&self) -> Option<Expr> {
        support::child(self.syntax())
    }

    pub fn alias(&self) -> Option<Ident> {
        support::child(self.syntax())
    }
}

ast_node!(WhereExpr);
impl WhereExpr {
    pub fn input(&self) -> Option<Expr> {
        support::nth_child(self.syntax(), 0)
    }

    pub fn predicate(&self) -> Option<Expr> {
        support::nth_child(self.syntax(), 1)
    }
}

ast_node!(DistinctExpr);
impl DistinctExpr {
    pub fn input(&self) -> Option<Expr> {
        support::child(self.syntax())
    }
}

ast_node!(DropExpr);
impl DropExpr {
    pub fn input(&self) -> Option<Expr> {
        support::child(self.syntax())
    }

    pub fn columns(&self) -> impl Iterator<Item = Ident> + '_ {
        support::children(self.syntax())
    }
}

ast_node!(RenameExpr);
impl RenameExpr {
    pub fn input(&self) -> Option<Expr> {
        support::child(self.syntax())
    }

    pub fn items(&self) -> impl Iterator<Item = RenameItem> + '_ {
        support::children(self.syntax())
    }
}

ast_node!(RenameItem);
impl RenameItem {
    pub fn from(&self) -> Option<Ident> {
        support::nth_child(self.syntax(), 0)
    }

    pub fn to(&self) -> Option<Ident> {
        support::nth_child(self.syntax(), 1)
    }
}

ast_node!(ExtendExpr);
impl ExtendExpr {
    pub fn input(&self) -> Option<Expr> {
        support::child(self.syntax())
    }

    pub fn items(&self) -> impl Iterator<Item = SelectItem> + '_ {
        support::children(self.syntax())
    }
}

ast_enum!(Literal, {
    BoolLiteral,
    IntLiteral,
    FloatLiteral,
    StringLiteral,
});

ast_node!(BoolLiteral);
impl BoolLiteral {
    pub fn value(&self) -> Option<bool> {
        self.0.first_token()?.text().parse().ok()
    }
}

ast_node!(IntLiteral);
impl IntLiteral {
    pub fn value(&self) -> Option<u64> {
        self.0.first_token()?.text().parse().ok()
    }
}

ast_node!(FloatLiteral);
impl FloatLiteral {
    pub fn value(&self) -> Option<f64> {
        self.0.first_token()?.text().parse().ok()
    }
}

ast_node!(StringLiteral);
impl StringLiteral {
    pub fn value(&self) -> Option<String> {
        let text = self.0.first_token()?.text().to_string();
        let unquoted = text
            .strip_prefix('"')
            .and_then(|inner| inner.strip_suffix('"'))
            .unwrap_or(text.as_str());
        Some(unquoted.to_string())
    }
}
