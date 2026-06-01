#include "HirBuilderGenerator.h"

#include "utils/SchemaUtils.h"
#include "utils/TreeUtils.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/TableGen/Error.h>
#include <llvm/TableGen/Record.h>

#include <algorithm>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace yuzu::tools {

namespace {

/// Sort defs by source position so emitted factory methods follow `.td`
/// declaration order. `getAllDerivedDefinitions` returns them alphabetically.
bool byLoc(const llvm::Record *a, const llvm::Record *b) {
  return a->getLoc().front().getPointer() < b->getLoc().front().getPointer();
}

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
//       return new (allocator) Foo(allocId(), ...);
//     }
//
//   private:
//     HirId allocId() { return HirId{nextId++}; }
//
//     uint32_t nextId = 0;
//     llvm::BumpPtrAllocator allocator;
//   };

/// Parameter type for `kind` on a builder method. Mirrors the node
/// ctor's param convention: non-trivial `Custom`/`Val` (e.g.
/// `std::u32string`) is taken by `const T &`, everything else by value.
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
          if (k.isTrivial) {
            return k.typeName;
          }
          return "const " + k.typeName + " &";
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

/// Walk `record`'s `Parent` chain (excluding `record` itself) and
/// collect each level's `Fields` innermost-to-outermost. Mirrors
/// `HirNodeGenerator::gatherInheritedFields`.
std::vector<NamedField> gatherInheritedFields(const llvm::Record *record) {
  std::vector<NamedField> out;
  const llvm::Record *cur = record;
  while (true) {
    if (!cur->getValue("Parent")) {
      break;
    }
    cur = cur->getValueAsDef("Parent");
    const auto fields = parseFields(cur);
    out.insert(out.end(), fields.begin(), fields.end());
  }
  return out;
}

/// True if `f` is the implicit `HirId` field on the Base — recognised
/// by its `Custom<Id>` schema type. The builder fills this in via
/// `allocId()`, not from a caller argument.
bool isIdField(const NamedField &f) {
  if (const auto *c = std::get_if<Custom>(&f.kind)) {
    return c->typeName == "Id";
  }
  return false;
}

void emitMakeMethod(CodeFormatter &fmt, const llvm::Record *node) {
  const std::string name = node->getName().str();
  const std::vector<NamedField> ownFields = parseFields(node);
  const std::vector<NamedField> inheritedFields = gatherInheritedFields(node);

  // Constructor expects: (...own, ...inherited). Factory exposes the
  // same order, but auto-supplies any `Id`-typed inherited field via
  // `allocId()` instead of asking the caller.
  std::string params;
  auto appendParam = [&](const NamedField &f) {
    if (isIdField(f)) {
      return;
    }
    if (!params.empty()) {
      params += ", ";
    }
    params += paramType(f.kind);
    params += " ";
    params += f.name;
  };
  for (const NamedField &f : ownFields) {
    appendParam(f);
  }
  for (const NamedField &f : inheritedFields) {
    appendParam(f);
  }

  fmt.linef("const {0} *make{0}({1}) {{", name, params);
  {
    auto body = fmt.block();
    std::string args;
    auto appendArg = [&](const NamedField &f) {
      if (!args.empty()) {
        args += ", ";
      }
      args += isIdField(f) ? "allocId()" : ctorArg(f);
    };
    for (const NamedField &f : ownFields) {
      appendArg(f);
    }
    for (const NamedField &f : inheritedFields) {
      appendArg(f);
    }
    fmt.linef("return new (allocator) {0}({1});", name, args);
  }
  fmt.line("}");
  fmt.line("");
}

} // namespace

void HirBuilderGenerator::generate(const llvm::RecordKeeper &records) {
  const std::string ns = findNamespace(records, "Base");
  std::vector<const llvm::Record *> nodes =
      records.getAllDerivedDefinitions("Node");
  std::sort(nodes.begin(), nodes.end(), byLoc);

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
    fmt.line("HirId allocId() { return HirId{nextId++}; }");
    fmt.line("");
    fmt.line("uint32_t nextId = 0;");
    fmt.line("llvm::BumpPtrAllocator allocator;");
  }
  fmt.line("};");
  fmt.line("");
  fmt.linef("} // namespace {0}", ns);
}

} // namespace yuzu::tools
