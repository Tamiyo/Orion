#ifndef YUZU_HIR_TYPES_TYPEUNIFIER_H
#define YUZU_HIR_TYPES_TYPEUNIFIER_H

#include "yuzu/Hir/Types/Type.h"
#include "yuzu/Hir/Types/TypeFactory.h"

#include <llvm/Support/Allocator.h>

#include <cstdint>
#include <vector>

namespace yuzu::hir {

class TypeUnifier {
public:
  explicit TypeUnifier(TypeFactory &types) : types(types) {}

  TypeUnifier(const TypeUnifier &) = delete;
  TypeUnifier &operator=(const TypeUnifier &) = delete;

  const InferTy *makeTypeHole(InferKind kind) {
    const auto id = static_cast<InferId>(filled.size());
    filled.push_back(nullptr);
    return new (arena.Allocate<InferTy>()) InferTy(id, kind);
  }

  bool unify(const Type *a, const Type *b) {
    a = find(a);
    b = find(b);

    if (a == b) {
      return true;
    }

    const auto *ha = InferTy::cast(a);
    const auto *hb = InferTy::cast(b);

    // two empty holes → merge their groups
    if (ha && hb) {
      return merge(ha, hb);
    }

    // hole + concrete
    if (ha) {
      return fill(ha, b);
    }

    // concrete + hole
    if (hb) {
      return fill(hb, a);
    }

    // two distinct concrete types
    return false;
  }

  const Type *resolve(const Type *t) {
    const auto *r = find(t);
    const auto *hole = InferTy::cast(r);

    if (!hole) {
      return r;
    }

    switch (hole->getInferKind()) {
    case InferKind::Int:
      return types.getInt64();
    case InferKind::Float:
      return types.getFloat64();
    case InferKind::General:
      return types.getError();
    }
  }

private:
  static uint32_t index(const InferTy *type) {
    return static_cast<uint32_t>(type->getId());
  }

  const Type *find(const Type *t) {
    const auto *hole = InferTy::cast(t);
    if (!hole) {
      return t;
    }

    const Type *filling = filled[index(hole)];
    if (!filling) {
      return t;
    }

    const Type *root = find(filling); // a hole may point at another hole
    filled[index(hole)] = root;       // path compression
    return root;
  }

  // Fill a hole with a concrete type, within its kind.
  bool fill(const InferTy *hole, const Type *concrete) {
    const InferKind kind = hole->getInferKind();
    const bool ok = kind == InferKind::General ||
                    (kind == InferKind::Int && concrete->isInt()) ||
                    (kind == InferKind::Float && concrete->isFloat());
    if (!ok) {
      return false;
    }

    filled[index(hole)] = concrete;
    return true;
  }

  // Merge two empty holes into one group. The root keeps the more specific
  // kind — a General hole yields to an Int/Float partner; Int vs Float clash.
  bool merge(const InferTy *a, const InferTy *b) {
    const InferKind ka = a->getInferKind();
    const InferKind kb = b->getInferKind();
    if (ka != kb && ka != InferKind::General && kb != InferKind::General) {
      return false; // Int vs Float
    }
    // Point one hole at the other so the root carries the non-General kind.
    if (kb == InferKind::General) {
      filled[index(b)] = a; // b → a (a is the root)
    } else {
      filled[index(a)] = b; // a → b (b is the root)
    }
    return true;
  }

  llvm::BumpPtrAllocator arena;
  std::vector<const Type *> filled;
  TypeFactory &types;
};

} // namespace yuzu::hir

#endif // YUZU_HIR_TYPES_UNIFICATIONTABLE_H