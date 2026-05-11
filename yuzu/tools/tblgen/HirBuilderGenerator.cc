#include "HirBuilderGenerator.h"

#include "utils/SchemaUtils.h"
#include "utils/TreeUtils.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/TableGen/Error.h>
#include <llvm/TableGen/Record.h>

#include <cstddef>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace yuzu::tools {

namespace {

//===----------------------------------------------------------------------===//
// HirBuilder layout
//===----------------------------------------------------------------------===//
//
//   class HirBuilder {
//   public:
//     HirBuilder() = default;
//     HirBuilder(const HirBuilder &) = delete;
//     HirBuilder &operator=(const HirBuilder &) = delete;
//
//     const Foo *makeFoo(...fields) {
//       return new (allocator) Foo(...);
//     }
//
//   private:
//     llvm::BumpPtrAllocator allocator;
//   };

/// Parameter type for `kind` on a builder method.
std::string paramType(const FieldKind &kind) {
  return std::visit(
      [](const auto &k) -> std::string {
        using T = std::decay_t<decltype(k)>;
        if constexpr (std::is_same_v<T, Child>) {
          return "const " + k.typeName + " *";
        } else if constexpr (std::is_same_v<T, Children>) {
          return "llvm::ArrayRef<const " + k.typeName + " *>";
        } else if constexpr (std::is_same_v<T, Custom> ||
                             std::is_same_v<T, Val>) {
          return k.typeName;
        } else {
          llvm::PrintFatalError(
              "yuzu-tblgen: HirBuilderGenerator has no parameter emitter for "
              "this Field kind yet");
        }
      },
      kind);
}

/// What to pass to the node's ctor. `Children<T>` is copied into the
/// arena so the node's view outlives the caller's input array.
std::string ctorArg(const NamedField &f) {
  return std::visit(
      [&](const auto &k) -> std::string {
        using T = std::decay_t<decltype(k)>;
        if constexpr (std::is_same_v<T, Children>) {
          return f.name + ".copy(allocator)";
        } else if constexpr (std::is_same_v<T, Child> ||
                             std::is_same_v<T, Custom> ||
                             std::is_same_v<T, Val>) {
          return f.name;
        } else {
          llvm::PrintFatalError(
              "yuzu-tblgen: HirBuilderGenerator has no ctor-arg emitter for "
              "this Field kind yet");
        }
      },
      f.kind);
}

void emitMakeMethod(CodeFormatter &fmt, const llvm::Record *node) {
  const std::string name = node->getName().str();
  const std::vector<NamedField> fields = parseFields(node);

  std::string params;
  for (std::size_t i = 0; i < fields.size(); ++i) {
    if (i != 0) {
      params += ", ";
    }
    params += paramType(fields[i].kind);
    params += " ";
    params += fields[i].name;
  }
  fmt.linef("const {0} *make{0}({1}) {{", name, params);
  {
    auto body = fmt.block();
    std::string args;
    for (std::size_t i = 0; i < fields.size(); ++i) {
      if (i != 0) {
        args += ", ";
      }
      args += ctorArg(fields[i]);
    }
    fmt.linef("return new (allocator) {0}({1});", name, args);
  }
  fmt.line("}");
  fmt.line("");
}

} // namespace

void HirBuilderGenerator::generate(const llvm::RecordKeeper &records) {
  const std::string ns = findNamespace(records, "Base");
  const std::vector<const llvm::Record *> nodes =
      records.getAllDerivedDefinitions("Node");

  fmt.linef("namespace {0} {{", ns);
  fmt.line("");
  fmt.line("class HirBuilder {");
  fmt.line("public:");
  {
    auto body = fmt.block();
    fmt.line("HirBuilder() = default;");
    fmt.line("HirBuilder(const HirBuilder &) = delete;");
    fmt.line("HirBuilder &operator=(const HirBuilder &) = delete;");
    fmt.line("");
    for (const llvm::Record *n : nodes) {
      emitMakeMethod(fmt, n);
    }
  }
  fmt.line("private:");
  {
    auto body = fmt.block();
    fmt.line("llvm::BumpPtrAllocator allocator;");
  }
  fmt.line("};");
  fmt.line("");
  fmt.linef("} // namespace {0}", ns);
}

} // namespace yuzu::tools
