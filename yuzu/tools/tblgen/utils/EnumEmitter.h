#ifndef YUZU_TOOLS_TBLGEN_UTILS_ENUM_EMITTER_H
#define YUZU_TOOLS_TBLGEN_UTILS_ENUM_EMITTER_H

#include "CodeFormatter.h"
#include "TreeUtils.h"

#include <llvm/TableGen/Record.h>

#include <string>

namespace yuzu::tools {

/// Emit each `Native` def as a C++ `using` alias so `Val<NativeDef>` and
/// `Custom<NativeDef>` accessors can name it by its schema name.
inline void emitNatives(CodeFormatter &fmt,
                        const llvm::RecordKeeper &records) {
  bool emitted = false;
  for (const llvm::Record *r : records.getAllDerivedDefinitions("Native")) {
    const Native n = parseNative(r);
    fmt.linef("using {0} = {1};", r->getName().str(), n.name);
    emitted = true;
  }
  if (emitted) {
    fmt.line("");
  }
}

/// Emit each `Enum` def in `records` as a C++ `enum class` followed by an
/// `inline std::string asString(EnumType)` that maps each case to its
/// name. Ordered before any class definitions so `Custom<Enum>:$f`
/// accessors can name the type.
inline void emitEnums(CodeFormatter &fmt,
                      const llvm::RecordKeeper &records) {
  for (const llvm::Record *r : records.getAllDerivedDefinitions("Enum")) {
    const Enum e = parseEnum(r);
    const std::string name = r->getName().str();

    fmt.linef("enum class [[nodiscard]] {0} : {1} {{", name, e.type);
    {
      auto body = fmt.block();
      for (const EnumCase &c : e.cases) {
        fmt.linef("{0},", c.name);
      }
    }
    fmt.line("};");
    fmt.line("");

    fmt.linef("inline std::string asString({0} value) {{", name);
    {
      auto body = fmt.block();
      fmt.line("switch (value) {");
      for (const EnumCase &c : e.cases) {
        fmt.linef("case {0}::{1}: return \"{1}\";", name, c.name);
      }
      fmt.line("}");
      fmt.line("");
      fmt.line("util::yuzu_unreachable();");
    }
    fmt.line("}");
    fmt.line("");
  }
}

} // namespace yuzu::tools

#endif // YUZU_TOOLS_TBLGEN_UTILS_ENUM_EMITTER_H
