#ifndef YUZU_TOOLS_TBLGEN_HIR_VISITOR_GENERATOR_H
#define YUZU_TOOLS_TBLGEN_HIR_VISITOR_GENERATOR_H

#include "CodeGenerator.h"

#include <llvm/Support/raw_ostream.h>
#include <llvm/TableGen/Record.h>

namespace yuzu::tools {

/// Emits a CRTP visitor template for the HIR tree. For every concrete
/// `Node` the generator emits three overridable hooks: `traverseX` —
/// default walks children then visits, `walkX` — default recurses into
/// every schema-declared `Child` / `Children` field, and `visitX` —
/// default no-op. A top-level `visit(const HirNode *)` switch-dispatches
/// on `HirKind` to the matching `traverseX`. Users derive
/// `class Foo : public HirVisitor<Foo>` and override whichever hook
/// matches the granularity they need.
class HirVisitorGenerator final : public CodeGenerator {
public:
  explicit HirVisitorGenerator(llvm::raw_ostream &os) : CodeGenerator(os) {}

protected:
  void generate(const llvm::RecordKeeper &records) override;
};

} // namespace yuzu::tools

#endif // YUZU_TOOLS_TBLGEN_HIR_VISITOR_GENERATOR_H
