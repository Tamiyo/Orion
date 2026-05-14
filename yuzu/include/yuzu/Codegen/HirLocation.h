#ifndef YUZU_CODEGEN_HIR_LOCATION_H
#define YUZU_CODEGEN_HIR_LOCATION_H

#include "yuzu/Hir/Hir.h"

#include "mlir/IR/Location.h"
#include "mlir/IR/MLIRContext.h"

#include <cstdint>
#include <optional>
#include <type_traits>

namespace yuzu::codegen {

/// Tag type used as the `TypeID` for `OpaqueLoc`s carrying a `HirId`.
/// Never instantiated — only its `TypeID` is consulted to distinguish our
/// locations from other `OpaqueLoc` users.
struct HirIdLocTag;

/// Encode `id` into an `mlir::OpaqueLoc` so the HIR origin travels with
/// each MLIR op through every pass and lowering. Diagnostics fired on
/// ops built this way can be looped back through `HirSourceMap` →
/// `ast::AstNode` → real source span.
inline mlir::Location locFor(mlir::MLIRContext &ctx, hir::HirId id) {
  const auto raw =
      static_cast<uintptr_t>(static_cast<std::underlying_type_t<hir::HirId>>(id));
  return mlir::OpaqueLoc::get<HirIdLocTag *>(
      reinterpret_cast<HirIdLocTag *>(raw), &ctx);
}

/// Decode the `HirId` carried by `loc`. Returns `std::nullopt` if `loc`
/// is not an `OpaqueLoc` tagged with `HirIdLocTag *` (`UnknownLoc`,
/// `FileLineColLoc`, or an `OpaqueLoc` from another payload kind).
inline std::optional<hir::HirId> hirIdFromLoc(mlir::Location loc) {
  const auto opaque = mlir::dyn_cast<mlir::OpaqueLoc>(loc);
  if (!opaque) {
    return std::nullopt;
  }
  if (opaque.getUnderlyingTypeID() != mlir::TypeID::get<HirIdLocTag *>()) {
    return std::nullopt;
  }
  const auto raw = opaque.getUnderlyingLocation();
  return static_cast<hir::HirId>(
      static_cast<std::underlying_type_t<hir::HirId>>(raw));
}

} // namespace yuzu::codegen

#endif // YUZU_CODEGEN_HIR_LOCATION_H
