#ifndef YUZU_HIR_HIRCONTEXT_H
#define YUZU_HIR_HIRCONTEXT_H

#include "yuzu/Ast/Ast.h"
#include "yuzu/Ast/AstSpan.h"
#include "yuzu/Diagnostics/DiagnosticsEngine.h"
#include "yuzu/Diagnostics/Span.h"
#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/HirBuilder.h"
#include "yuzu/Hir/Resolve/HirSymbolTable.h"
#include "yuzu/Hir/Types/Adjustment.h"
#include "yuzu/Hir/Types/TypeContext.h"
#include "yuzu/Util/SideTable.h"
#include "yuzu/Util/StringInterner.h"

#include <llvm/ADT/Twine.h>

namespace yuzu::hir {
/// Maps each HIR node's `HirId` back to the AST view it was lowered from.
/// Populated by the lowerer; consumed by diagnostics, LSP, and any pass
/// that needs to point at source. Synthetic HIR nodes (no AST origin)
/// leave their slot unbound and `get` returns `std::nullopt`.
using HirSourceTable = util::SideTable<HirId, ast::AstNode>;

/// Side table of pending coercions, keyed by the `HirId` of the
/// expression whose value needs adjusting. Populated by `TypeChecker`,
/// consumed by codegen when it visits each expression — codegen
/// generates the value as the expression's own type, then wraps in the
/// adjustment's cast op if an entry exists.
using HirAdjustmentTable = util::SideTable<HirId, Adjustment>;

/// Unresolved type annotations, keyed by the annotated node's `HirId`.
/// Lowering records the raw `ast::TypeExpr`; `TypeInferrer` resolves it
/// against scope (a generic `T` is only meaningful in the scoped pass).
using HirTypeAnnotationTable = util::SideTable<HirId, ast::TypeExpr>;

class HirContext final {
public:
  explicit HirContext(diagnostics::DiagnosticsEngine &diagnostics,
                      diagnostics::SourceId sourceId)
      : diagnostics(diagnostics), sourceId(sourceId) {}

  HirBuilder &getBuilder() { return builder; }
  HirSymbolTable &getSymbolTable() { return symbolTable; }
  HirSourceTable &getSourceTable() { return sourceTable; }
  HirAdjustmentTable &getAdjustments() { return adjustments; }
  HirTypeAnnotationTable &getTypeAnnotations() { return typeAnnotations; }
  TypeContext &getTypeContext() { return typeContext; }
  util::StringInterner &getStringInterner() { return stringInterner; }

  /// Rebind the source id used by `spanFor`/`error` for any HIR nodes
  /// lowered after this point.
  void setSourceId(diagnostics::SourceId id) { sourceId = id; }

  diagnostics::DiagnosticBuilder error(const HirNode *node,
                                       const llvm::Twine &message) {
    return diagnostics.error(spanFor(node), message);
  }

  /// Diagnostic anchored at the smallest span covering `nodes`. Useful
  /// when an operator's resolve sees only its operands and wants to
  /// point at "everything that participated in the call" — the operator
  /// token sitting between them is included by virtue of running from
  /// the first node's start to the last node's end.
  template <typename T>
  diagnostics::DiagnosticBuilder error(llvm::ArrayRef<const T *> nodes,
                                       const llvm::Twine &message) {
    return diagnostics.error(spanFor(nodes), message);
  }

  diagnostics::Span spanFor(HirId id) const {
    if (const auto origin = sourceTable.get(id)) {
      const auto range = ast::tightRange(*origin);
      return diagnostics::Span{sourceId, range.start, range.end};
    }
    return diagnostics::Span{sourceId, 0, 0};
  }

  diagnostics::Span spanFor(const HirNode *node) const {
    return spanFor(node->getId());
  }

  template <typename T>
  diagnostics::Span spanFor(llvm::ArrayRef<const T *> nodes) const {
    static_assert(std::is_base_of_v<HirNode, T>,
                  "spanFor expects pointers to HIR nodes");
    if (nodes.empty()) {
      return diagnostics::Span{sourceId, 0, 0};
    }
    const auto first = spanFor(nodes.front()->getId());
    const auto last = spanFor(nodes.back()->getId());
    return diagnostics::Span{sourceId, first.start, last.end};
  }

private:
  HirBuilder builder;
  HirSymbolTable symbolTable{*this};
  HirSourceTable sourceTable;
  HirAdjustmentTable adjustments;
  HirTypeAnnotationTable typeAnnotations;
  util::StringInterner stringInterner;
  TypeContext typeContext{stringInterner};

  diagnostics::DiagnosticsEngine &diagnostics;
  diagnostics::SourceId sourceId;
};
} // namespace yuzu::hir

#endif // YUZU_HIR_HIRCONTEXT_H
