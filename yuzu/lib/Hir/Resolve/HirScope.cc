#include "yuzu/Hir/Resolve/HirScope.h"

#include "yuzu/Hir/Resolve/HirSymbolTable.h"

namespace yuzu::hir {
HirScope::~HirScope() { symbolTable.popScope(); }
} // namespace yuzu::hir