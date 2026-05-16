#ifndef YUZU_HIR_HIRPRINTER_H
#define YUZU_HIR_HIRPRINTER_H

#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/HirContext.h"
#include "yuzu/Hir/Types/Adjustment.h" // IWYU pragma: keep (printer reads adjustments)
#include "yuzu/Hir/Types/Type.h" // IWYU pragma: keep (printer reads type kinds)
#include "yuzu/Util/ErrorHandling.h" // IWYU pragma: keep
#include "yuzu/Util/Unicode.h" // IWYU pragma: keep (printers use writeUtf8)

#include <llvm/Support/raw_ostream.h>

#include <cstddef> // IWYU pragma: keep
#include <string>

// Generated per-Node `printX` free functions plus the `printNode`
// dispatcher. Emits its own `namespace yuzu::hir { ... }` block. Each
// printer takes a `HirContext &` so it can consult side tables for
// inline type / adjustment annotations.
#include "yuzu/Hir/HirPrinter.h.inc" // IWYU pragma: export

namespace yuzu::hir {

/// Pretty-prints an HIR tree as an indented kind tree, optionally
/// annotated with each node's type and pending adjustment. Annotations
/// appear inline next to the node name when the side tables have
/// entries — pre-typecheck trees print structurally with no
/// annotations. One line per item (node name OR scalar field),
/// two-space indent per depth, no trailing newline. Example after
/// typechecking `1 + 2.0`:
///
///   BinaryExpr : Float64
///     IntLit : Int64 → cast Float64
///       value=1
///     op=Add
///     FloatLit : Float64
///       value=2.0
class [[nodiscard]] HirPrinter final {
public:
  explicit HirPrinter(llvm::raw_ostream &os, HirContext &ctx)
      : os(os), ctx(ctx) {}

  HirPrinter() = delete;

  void print(const HirNode *node) { printNode(os, ctx, node, 0); }

  static std::string printToString(HirContext &ctx, const HirNode *node) {
    std::string out;
    llvm::raw_string_ostream stream(out);
    HirPrinter(stream, ctx).print(node);
    return out;
  }

private:
  llvm::raw_ostream &os;
  HirContext &ctx;
};

} // namespace yuzu::hir

#endif // YUZU_HIR_HIRPRINTER_H
