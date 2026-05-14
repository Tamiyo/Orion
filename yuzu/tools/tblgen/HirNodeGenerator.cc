#include "HirNodeGenerator.h"

#include "utils/EnumEmitter.h"
#include "utils/SchemaUtils.h"
#include "utils/StringUtils.h"
#include "utils/TreeUtils.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/TableGen/Error.h>
#include <llvm/TableGen/Record.h>

#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace yuzu::tools {

namespace {

//===----------------------------------------------------------------------===//
// Class definitions
//===----------------------------------------------------------------------===//
//
// Each emitted HIR type owns its fields by value (or by pointer into the
// arena for `Child` / `Children`), e.g.:
//
//   class HirNode {
//   public:
//     HirKind getKind() const;
//   protected:
//     explicit HirNode(HirKind);
//     HirKind kind;
//   };
//
//   class Foo : public Bar {
//   public:
//     Foo(...fields);
//     static bool isA(HirKind);
//     static const Foo *cast(const HirNode *);
//     ... accessors ...
//   private:
//     ...field storage...
//   };

const llvm::Record *findBase(const llvm::RecordKeeper &records) {
  const llvm::Record *base = nullptr;
  for (const llvm::Record *record : records.getAllDerivedDefinitions("Base")) {
    if (record->isSubClassOf("Variant")) {
      continue;
    }
    
    if (base) {
      llvm::PrintFatalError(record->getLoc(),
                            "yuzu-tblgen: multiple Base defs found ('" +
                                base->getName().str() + "' and '" +
                                record->getName().str() + "')");
    }

    base = record;
  }
  
  if (!base) {
    llvm::PrintFatalError("yuzu-tblgen: no Base def found");
  }
  return base;
}

/// Storage type for `kind`, used for both the ctor parameter and the
/// private member.
std::string fieldStorageType(const FieldKind &kind) {
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
          llvm::PrintFatalError("yuzu-tblgen: HirNodeGenerator has no storage "
                                "emitter for this Field kind yet");
        }
      },
      kind);
}

/// Emit the grammar root: stores the `HirId` and `HirKind`, exposes
/// `getId` / `getKind`, and provides a `protected` ctor for subclasses.
void emitBaseClass(CodeFormatter &fmt, const llvm::Record *base) {
  const std::string name = base->getName().str();
  const llvm::StringRef summary = base->getValueAsString("Summary");

  if (!summary.empty()) {
    fmt.linef("/// {0}", summary);
  }
  fmt.linef("class [[nodiscard]] {0} {{", name);
  fmt.line("public:");
  {
    auto body = fmt.block();
    fmt.line("[[nodiscard]] HirId getId() const { return id; }");
    fmt.line("");
    fmt.line("[[nodiscard]] HirKind getKind() const { return kind; }");
  }
  fmt.line("");
  fmt.line("protected:");
  {
    auto body = fmt.block();
    fmt.linef("{0}(HirId id, HirKind kind) : id(id), kind(kind) {{}", name);
    fmt.linef("{0}() = delete;", name);
    fmt.line("");
    fmt.line("HirId id;");
    fmt.line("HirKind kind;");
  }
  fmt.line("};");
  fmt.line("");
}

/// Emit a Variant class. `isA` is a `HirKind` range check between the
/// `<V>_FIRST` / `<V>_LAST` sentinels.
void emitVariantClass(CodeFormatter &fmt, const llvm::Record *variant) {
  const std::string name = variant->getName().str();
  const std::string parentName =
      variant->getValueAsDef("Parent")->getName().str();
  const llvm::StringRef summary = variant->getValueAsString("Summary");
  const std::string sentinel = llvm::StringRef(name).upper();

  if (!summary.empty()) {
    fmt.linef("/// {0}", summary);
  }
  fmt.linef("class [[nodiscard]] {0} : public {1} {{", name, parentName);
  fmt.line("public:");
  {
    auto body = fmt.block();

    fmt.line("[[nodiscard]] static bool isA(HirKind kind) {");
    {
      auto inner = fmt.block();
      fmt.linef("return kind > HirKind::{0}_FIRST "
                "&& kind < HirKind::{0}_LAST;",
                sentinel);
    }
    fmt.line("}");
    fmt.line("");

    fmt.linef("[[nodiscard]] static const {0} *cast(const HirNode *node) {{",
              name);
    {
      auto inner = fmt.block();
      fmt.line("if (!isA(node->getKind())) {");
      fmt.line("  return nullptr;");
      fmt.line("}");
      fmt.linef("return static_cast<const {0} *>(node);", name);
    }
    fmt.line("}");
  }
  fmt.line("");
  fmt.line("protected:");
  {
    auto body = fmt.block();
    fmt.linef("{0}(HirId id, HirKind kind) : {1}(id, kind) {{}", name,
              parentName);
  }
  fmt.line("};");
  fmt.line("");
}

/// Emit a concrete Node class.
void emitNodeClass(CodeFormatter &fmt, const llvm::Record *node) {
  const std::string name = node->getName().str();
  const std::string parentName = node->getValueAsDef("Parent")->getName().str();
  const llvm::StringRef summary = node->getValueAsString("Summary");
  const std::vector<NamedField> fields = parseFields(node);

  if (!summary.empty()) {
    fmt.linef("/// {0}", summary);
  }
  fmt.linef("class [[nodiscard]] {0} : public {1} {{", name, parentName);
  fmt.line("public:");
  {
    auto body = fmt.block();

    // Constructor. `HirId` is always the first parameter; field
    // parameters follow. Fieldless nodes still need an explicit body so
    // the parent initializer fires.
    if (fields.empty()) {
      fmt.linef("explicit {0}(HirId id) : {1}(id, HirKind::{0}) {{}", name,
                parentName);
    } else {
      std::string params = "HirId id";
      for (const NamedField &f : fields) {
        params += ", ";
        params += fieldStorageType(f.kind);
        params += " ";
        params += f.name;
      }
      fmt.linef("{0}({1})", name, params);
      {
        auto inner = fmt.block();
        std::string init = parentName + "(id, HirKind::" + name + ")";
        for (const NamedField &f : fields) {
          init += ", " + f.name + "(" + f.name + ")";
        }
        fmt.linef(": {0} {{}", init);
      }
    }
    fmt.line("");

    fmt.linef("[[nodiscard]] static bool isA(HirKind kind) "
              "{{ return kind == HirKind::{0}; }",
              name);
    fmt.line("");

    fmt.linef("[[nodiscard]] static const {0} *cast(const HirNode *node) {{",
              name);
    {
      auto inner = fmt.block();
      fmt.line("if (!isA(node->getKind())) {");
      fmt.line("  return nullptr;");
      fmt.line("}");
      fmt.linef("return static_cast<const {0} *>(node);", name);
    }
    fmt.line("}");

    for (const NamedField &f : fields) {
      const std::string accessor = "get" + capitalize(f.name);
      const std::string type = fieldStorageType(f.kind);
      fmt.line("");
      fmt.linef("[[nodiscard]] {0} {1}() const {{ return {2}; }", type,
                accessor, f.name);
    }
  }

  if (!fields.empty()) {
    fmt.line("");
    fmt.line("private:");
    auto body = fmt.block();
    for (const NamedField &f : fields) {
      fmt.linef("{0} {1};", fieldStorageType(f.kind), f.name);
    }
  }

  fmt.line("};");
  fmt.line("");
}

} // namespace

void HirNodeGenerator::generate(const llvm::RecordKeeper &records) {
  const std::string ns = findNamespace(records, "Base");
  const llvm::Record *base = findBase(records);

  const std::vector<const llvm::Record *> variants =
      records.getAllDerivedDefinitions("Variant");
  const std::vector<const llvm::Record *> nodes =
      records.getAllDerivedDefinitions("Node");

  fmt.linef("namespace {0} {{", ns);
  fmt.line("");

  // Native aliases and enums first — `Val<NativeDef>` and `Custom<...>`
  // accessors need them complete when the class that owns them is parsed.
  emitNatives(fmt, records);
  emitEnums(fmt, records);

  // Grammar root next: every Variant/Node inherits transitively from it.
  emitBaseClass(fmt, base);

  // Forward-declare every Variant and Node so accessor signatures (and
  // ArrayRef element types) can name them in either order.
  for (const llvm::Record *v : variants) {
    fmt.linef("class {0};", v->getName().str());
  }
  for (const llvm::Record *n : nodes) {
    fmt.linef("class {0};", n->getName().str());
  }
  if (!variants.empty() || !nodes.empty()) {
    fmt.line("");
  }

  for (const llvm::Record *v : variants) {
    emitVariantClass(fmt, v);
  }

  for (const llvm::Record *n : nodes) {
    emitNodeClass(fmt, n);
  }

  fmt.linef("} // namespace {0}", ns);
}

} // namespace yuzu::tools
