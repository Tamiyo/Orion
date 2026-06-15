#ifndef YUZU_ANF_ANFCONTEXT_H
#define YUZU_ANF_ANFCONTEXT_H

#include "yuzu/Anf/Anf.h"
#include "yuzu/Anf/AnfBuilder.h"
#include "yuzu/Diagnostics/DiagnosticBuilder.h"
#include "yuzu/Diagnostics/DiagnosticsEngine.h"
#include "yuzu/Diagnostics/Span.h"
#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/HirContext.h"
#include "yuzu/Util/SideTable.h"

#include <llvm/ADT/Twine.h>

namespace yuzu::anf {
using AnfSourceTable = util::SideTable<AnfId, const hir::HirNode *>;
using AnfTempTable = util::SideTable<AnfId, const Expr *>;

class AnfContext final {
public:
  AnfContext(diagnostics::DiagnosticsEngine &diagnostics,
             diagnostics::SourceId sourceId, hir::HirContext &hirContext)
      : diagnostics(diagnostics), sourceId(sourceId), hirContext(hirContext) {}

  AnfBuilder &getBuilder() { return builder; }
  AnfSourceTable &getAnfSourceTable() { return anfSourceTable; }
  AnfTempTable &getAnfTempTable() { return anfTempTable; }

  template <typename Method, typename... Args>
  auto build(const hir::HirNode *origin, Method method, Args &&...args) {
    auto *node = (builder.*method)(std::forward<Args>(args)...);
    anfSourceTable.bind(node->getId(), origin);
    return node;
  }

  /// The source span of `node`, resolved through its HIR origin. Falls back to
  /// a file-level span when the node has no recorded origin — e.g. a constant
  /// synthesized during reduction.
  [[nodiscard]] diagnostics::Span spanFor(const AnfNode *node) const {
    if (const auto *origin = anfSourceTable.get(node->getId())) {
      return hirContext.spanFor(*origin);
    }
    return diagnostics::Span{sourceId, 0, 0};
  }

  /// Begin a warning anchored at `node` (via its HIR origin).
  diagnostics::DiagnosticBuilder warning(const AnfNode *node,
                                         const llvm::Twine &message) {
    return diagnostics.warning(spanFor(node), message);
  }

private:
  AnfBuilder builder;
  AnfSourceTable anfSourceTable;
  AnfTempTable anfTempTable;

  diagnostics::DiagnosticsEngine &diagnostics;
  diagnostics::SourceId sourceId;
  hir::HirContext &hirContext;
};

} // namespace yuzu::anf

#endif // YUZU_ANF_ANFCONTEXT_H
