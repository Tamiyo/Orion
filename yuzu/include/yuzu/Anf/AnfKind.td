//===----------------------------------------------------------------------===//
// AnfKind umbrella.
//===----------------------------------------------------------------------===//
//
// Single TableGen entry point for `yuzu-tblgen -gen-anf-kind-decls`. The Kind
// generator emits the `AnfKind` RTTI enum independently of the node hierarchy,
// so it gets its own entry point even though it only pulls in `Anf.td`.
//
//===----------------------------------------------------------------------===//

include "yuzu/Anf/Anf.td"
