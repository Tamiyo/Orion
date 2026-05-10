#ifndef YUZU_TOOLS_TBLGEN_UTILS_SCHEMA_UTILS_H
#define YUZU_TOOLS_TBLGEN_UTILS_SCHEMA_UTILS_H

#include <llvm/ADT/StringRef.h>
#include <llvm/TableGen/Error.h>
#include <llvm/TableGen/Record.h>

#include <string>

namespace yuzu::tools {

/// Find the C++ namespace declared by exactly one def derived from
/// `className` via its `Namespace` field. Conflicting non-empty values, or
/// no def declaring one at all, is a fatal error — every consumer of this
/// helper expects a single canonical namespace.
inline std::string findNamespace(const llvm::RecordKeeper &records,
                                 llvm::StringRef className) {
  std::string ns;
  for (const llvm::Record *r : records.getAllDerivedDefinitions(className)) {
    const llvm::StringRef candidate = r->getValueAsString("Namespace");
    if (candidate.empty())
      continue;
    if (ns.empty()) {
      ns = candidate.str();
    } else if (candidate != ns) {
      llvm::PrintFatalError(r->getLoc(),
                            "yuzu-tblgen: conflicting Namespace across " +
                                className.str() + " defs ('" + ns + "' vs '" +
                                candidate.str() + "')");
    }
  }
  if (ns.empty()) {
    llvm::PrintFatalError("yuzu-tblgen: no " + className.str() +
                          " def declares a non-empty Namespace");
  }
  return ns;
}

} // namespace yuzu::tools

#endif // YUZU_TOOLS_TBLGEN_UTILS_SCHEMA_UTILS_H
