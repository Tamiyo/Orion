#ifndef YUZU_HIR_RESOLVE_BINDING_H
#define YUZU_HIR_RESOLVE_BINDING_H

#include "yuzu/Hir/Hir.h"

#include <variant>

namespace yuzu::hir {
using Binding = std::variant<const LetStmt *, const Param *, const FuncStmt *,
                             const Ident *>;
} // namespace yuzu::hir

#endif
