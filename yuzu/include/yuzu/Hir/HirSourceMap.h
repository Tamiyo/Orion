#ifndef YUZU_HIR_HIR_SOURCE_MAP_H
#define YUZU_HIR_HIR_SOURCE_MAP_H

#include "yuzu/Ast/Ast.h"
#include "yuzu/Hir/Hir.h"
#include "yuzu/Util/LookupTable.h"

namespace yuzu::hir {

/// Maps each HIR node's `HirId` back to the AST view it was lowered from.
/// Populated by the lowerer; consumed by diagnostics, LSP, and any pass
/// that needs to point at source. Synthetic HIR nodes (no AST origin)
/// leave their slot unbound and `get` returns `std::nullopt`.
using HirSourceMap = util::LookupTable<HirId, ast::AstNode>;

} // namespace yuzu::hir

#endif // YUZU_HIR_HIR_SOURCE_MAP_H
