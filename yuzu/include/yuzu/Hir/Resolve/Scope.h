#ifndef YUZU_HIR_RESOLVE_SCOPE_H
#define YUZU_HIR_RESOLVE_SCOPE_H

#include "yuzu/Hir/Hir.h"

#include <llvm/ADT/DenseMap.h>

#include <cstdint>
#include <vector>

namespace yuzu::hir {
enum class [[nodiscard]] ScopeKind : uint8_t { Block };

class [[nodiscard]] ScopeHandler {
public:
  class [[nodiscard]] Scope {
  public:
    explicit Scope(ScopeHandler &handler, ScopeKind kind)
        : handler(handler), kind(kind) {}

    ~Scope() { handler.popScope(); }

    Scope(const Scope &) = delete;
    Scope &operator=(const Scope &) = delete;
    Scope(Scope &&) = delete;
    Scope &operator=(Scope &&) = delete;

    ScopeKind getKind() const { return kind; }

  private:
    llvm::DenseMap<HirNode, HirNode> bindings;
    ScopeHandler &handler;
    ScopeKind kind;
  };

  explicit ScopeHandler() {
    scopes.emplace_back(Scope(*this, ScopeKind::Block));
  }

  Scope &pushScope(ScopeKind kind) {
    scopes.emplace_back(Scope(*this, kind));
    return scopes.back();
  }

private:
  void popScope() { scopes.pop_back(); }

  std::vector<Scope> scopes;
};
} // namespace yuzu::hir

#endif
