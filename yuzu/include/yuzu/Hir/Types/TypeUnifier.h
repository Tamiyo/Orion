#ifndef YUZU_HIR_TYPES_TYPEUNIFIER_H
#define YUZU_HIR_TYPES_TYPEUNIFIER_H

#include "yuzu/Types/Type.h"
#include "yuzu/Types/TypeFactory.h"

#include <llvm/Support/Allocator.h>

#include <cstdint>
#include <vector>

namespace yuzu::hir {

class TypeUnifier {
public:
  explicit TypeUnifier(types::TypeFactory &types) : types(types) {}

  TypeUnifier(const TypeUnifier &) = delete;
  TypeUnifier &operator=(const TypeUnifier &) = delete;

  const types::InferType *makeTypeHole(types::InferKind kind) {
    const auto id = static_cast<types::InferId>(filled.size());
    filled.push_back(nullptr);
    return new (arena.Allocate<types::InferType>()) types::InferType(id, kind);
  }

  bool unify(const types::Type *a, const types::Type *b) {
    a = find(a);
    b = find(b);

    if (a == b) {
      return true;
    }

    const auto *ha = types::InferType::cast(a);
    const auto *hb = types::InferType::cast(b);

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

    // Type-parameter markers are interned by their declaration, so the
    // same
    // `[T]` is one pointer (already caught by `a == b` above) and distinct
    // declarations are distinct pointers. Two markers that aren't pointer-equal
    // denote different parameters (e.g. a nested `[U]` vs an enclosing `[T]`)
    // and must not unify.
    if (types::TypeParamType::cast(a) && types::TypeParamType::cast(b)) {
      return false;
    }

    // List types are interned by element, so `List[T]` and `List[U]` are
    // distinct pointers unless the elements are already identical — unify them
    // through the element (so `List[<infer>]` adopts `List[int32]`).
    const auto *la = types::ListType::cast(a);
    const auto *lb = types::ListType::cast(b);
    if (la && lb) {
      return unify(la->getElement(), lb->getElement());
    }

    // Function types aren't interned, so structurally-equal signatures are
    // distinct pointers — unify them component-wise (params then result).
    const auto *fa = types::FuncType::cast(a);
    const auto *fb = types::FuncType::cast(b);
    if (fa && fb) {
      if (fa->getArgTypes().size() != fb->getArgTypes().size()) {
        return false;
      }
      for (size_t i = 0; i < fa->getArgTypes().size(); ++i) {
        if (!unify(fa->getArgTypes()[i], fb->getArgTypes()[i])) {
          return false;
        }
      }
      return unify(fa->getReturnType(), fb->getReturnType());
    }

    // two distinct concrete types
    return false;
  }

  const types::Type *resolve(const types::Type *t) {
    const auto *r = find(t);
    const auto *hole = types::InferType::cast(r);

    if (!hole) {
      return r;
    }

    switch (hole->getInferKind()) {
    case types::InferKind::Int:
      return types.getInt64Type();
    case types::InferKind::Float:
      return types.getFloat64Type();
    case types::InferKind::General:
      return types.getErrorType();
    }
  }

private:
  static uint32_t index(const types::InferType *type) {
    return static_cast<uint32_t>(type->getId());
  }

  const types::Type *find(const types::Type *t) {
    const auto *hole = types::InferType::cast(t);
    if (!hole) {
      return t;
    }

    const types::Type *filling = filled[index(hole)];
    if (!filling) {
      return t;
    }

    const types::Type *root = find(filling); // a hole may point at another hole
    filled[index(hole)] = root;              // path compression
    return root;
  }

  // Fill a hole with a concrete type, within its kind.
  bool fill(const types::InferType *hole, const types::Type *concrete) {
    const types::InferKind kind = hole->getInferKind();
    const bool ok = kind == types::InferKind::General ||
                    (kind == types::InferKind::Int && concrete->isInt()) ||
                    (kind == types::InferKind::Float && concrete->isFloat());
    if (!ok) {
      return false;
    }

    filled[index(hole)] = concrete;
    return true;
  }

  // Merge two empty holes into one group. The root keeps the more specific
  // kind — a General hole yields to an Int/Float partner; Int vs Float clash.
  bool merge(const types::InferType *a, const types::InferType *b) {
    const types::InferKind ka = a->getInferKind();
    const types::InferKind kb = b->getInferKind();
    if (ka != kb && ka != types::InferKind::General &&
        kb != types::InferKind::General) {
      return false; // Int vs Float
    }
    // Point one hole at the other so the root carries the non-General kind.
    if (kb == types::InferKind::General) {
      filled[index(b)] = a; // b → a (a is the root)
    } else {
      filled[index(a)] = b; // a → b (b is the root)
    }
    return true;
  }

  llvm::BumpPtrAllocator arena;
  std::vector<const types::Type *> filled;
  types::TypeFactory &types;
};

} // namespace yuzu::hir

#endif // YUZU_HIR_TYPES_UNIFICATIONTABLE_H