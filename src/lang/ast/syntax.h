#ifndef LANG_AST_SYNTAX_H_
#define LANG_AST_SYNTAX_H_

#include "lang/parser/syntax_kind.h"
#include "syntax/rgtree/syntax.h"

namespace yuzu::lang {
using SyntaxNode = syntax::SyntaxNode<SyntaxKind>;
using SyntaxToken = syntax::SyntaxToken<SyntaxKind>;
}  // namespace yuzu::lang

#endif  // LANG_AST_SYNTAX_H_
