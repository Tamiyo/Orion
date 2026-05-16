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

/// Walk `record`'s `Parent` chain (excluding `record` itself) up to the
/// Base, collecting each level's `Fields` in **innermost-to-outermost**
/// order. So for `IntLit : Node<Literal>` where `Literal : Variant<Expr>`
/// and `Expr : Variant<HirNode>`, the returned list is
/// `Literal.Fields ++ Expr.Fields ++ HirNode.Fields`.
std::vector<NamedField> gatherInheritedFields(const llvm::Record *record) {
  std::vector<NamedField> out;
  const llvm::Record *cur = record;
  while (true) {
    if (!cur->getValue("Parent")) {
      break; // Base: no Parent — done.
    }
    cur = cur->getValueAsDef("Parent");
    const auto fields = parseFields(cur);
    out.insert(out.end(), fields.begin(), fields.end());
  }
  return out;
}

/// Render a comma-separated parameter list `<type> <name>` for `fields`.
/// `prefix`, if non-empty, is inserted as the first parameter.
std::string formatParams(const std::vector<NamedField> &fields,
                         const std::string &prefix = "") {
  std::string params = prefix;
  for (const NamedField &f : fields) {
    if (!params.empty()) {
      params += ", ";
    }
    params += fieldStorageType(f.kind);
    params += " ";
    params += f.name;
  }
  return params;
}

/// Emit the grammar root. The Base may declare schema Fields (e.g.
/// `Custom<Id>:$id`) that get treated like any other field — storage,
/// accessor, ctor param. `HirKind` is always implicit (every concrete
/// Node pins it via its parent constructor).
void emitBaseClass(CodeFormatter &fmt, const llvm::Record *base) {
  const std::string name = base->getName().str();
  const llvm::StringRef summary = base->getValueAsString("Summary");
  const std::vector<NamedField> ownFields = parseFields(base);

  if (!summary.empty()) {
    fmt.linef("/// {0}", summary);
  }
  fmt.linef("class [[nodiscard]] {0} {{", name);
  fmt.line("public:");
  {
    auto body = fmt.block();
    fmt.line("[[nodiscard]] HirKind getKind() const { return kind; }");
    for (const NamedField &f : ownFields) {
      const std::string accessor = "get" + capitalize(f.name);
      const std::string type = fieldStorageType(f.kind);
      fmt.line("");
      fmt.linef("[[nodiscard]] {0} {1}() const {{ return {2}; }", type,
                accessor, f.name);
    }
  }
  fmt.line("");
  fmt.line("protected:");
  {
    auto body = fmt.block();
    // Ctor: (HirKind kind, ...own fields). Stores kind + each own field.
    const std::string params = formatParams(ownFields, "HirKind kind");
    fmt.linef("{0}({1})", name, params);
    {
      auto inner = fmt.block();
      std::string init = "kind(kind)";
      for (const NamedField &f : ownFields) {
        init += ", " + f.name + "(" + f.name + ")";
      }
      fmt.linef(": {0} {{}", init);
    }
    fmt.linef("{0}() = delete;", name);
    fmt.line("");
    fmt.line("HirKind kind;");
    for (const NamedField &f : ownFields) {
      fmt.linef("{0} {1};", fieldStorageType(f.kind), f.name);
    }
  }
  fmt.line("};");
  fmt.line("");
}

/// Emit a Variant class. Variants can declare their own `Fields` (e.g.
/// `Expr` carrying a `Type *`) which contribute to the ctor signature
/// alongside whatever's inherited from further up the chain. `isA` is a
/// `HirKind` range check between the `<V>_FIRST` / `<V>_LAST` sentinels.
void emitVariantClass(CodeFormatter &fmt, const llvm::Record *variant) {
  const std::string name = variant->getName().str();
  const std::string parentName =
      variant->getValueAsDef("Parent")->getName().str();
  const llvm::StringRef summary = variant->getValueAsString("Summary");
  const std::string sentinel = llvm::StringRef(name).upper();
  const std::vector<NamedField> ownFields = parseFields(variant);
  const std::vector<NamedField> inheritedFields =
      gatherInheritedFields(variant);

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
    fmt.line("");

    fmt.linef("[[nodiscard]] {0}Kind get{0}Kind() const "
              "{{ return to{0}Kind(getKind()); }",
              name);

    // Accessors for own fields.
    for (const NamedField &f : ownFields) {
      const std::string accessor = "get" + capitalize(f.name);
      const std::string type = fieldStorageType(f.kind);
      fmt.line("");
      fmt.linef("[[nodiscard]] {0} {1}() const {{ return {2}; }", type,
                accessor, f.name);
    }
  }
  fmt.line("");
  fmt.line("protected:");
  {
    auto body = fmt.block();
    // Ctor signature: (HirKind kind, ...own, ...inherited).
    std::string params = "HirKind kind";
    for (const NamedField &f : ownFields) {
      params += ", ";
      params += fieldStorageType(f.kind);
      params += " ";
      params += f.name;
    }
    for (const NamedField &f : inheritedFields) {
      params += ", ";
      params += fieldStorageType(f.kind);
      params += " ";
      params += f.name;
    }
    fmt.linef("{0}({1})", name, params);
    {
      auto inner = fmt.block();
      // Forward kind + inherited up; store own.
      std::string init = parentName + "(kind";
      for (const NamedField &f : inheritedFields) {
        init += ", " + f.name;
      }
      init += ")";
      for (const NamedField &f : ownFields) {
        init += ", " + f.name + "(" + f.name + ")";
      }
      fmt.linef(": {0} {{}", init);
    }
  }

  if (!ownFields.empty()) {
    fmt.line("");
    fmt.line("private:");
    auto body = fmt.block();
    for (const NamedField &f : ownFields) {
      fmt.linef("{0} {1};", fieldStorageType(f.kind), f.name);
    }
  }
  fmt.line("};");
  fmt.line("");
}

/// Emit a concrete Node class. Constructor takes own fields first,
/// then inherited (variant chain) fields last. `HirKind` is pinned
/// from the node's own name and forwarded to the parent.
void emitNodeClass(CodeFormatter &fmt, const llvm::Record *node) {
  const std::string name = node->getName().str();
  const std::string parentName = node->getValueAsDef("Parent")->getName().str();
  const llvm::StringRef summary = node->getValueAsString("Summary");
  const std::vector<NamedField> ownFields = parseFields(node);
  const std::vector<NamedField> inheritedFields = gatherInheritedFields(node);

  if (!summary.empty()) {
    fmt.linef("/// {0}", summary);
  }
  fmt.linef("class [[nodiscard]] {0} : public {1} {{", name, parentName);
  fmt.line("public:");
  {
    auto body = fmt.block();

    // Constructor: (...own fields, ...inherited fields).
    std::string params;
    for (const NamedField &f : ownFields) {
      if (!params.empty()) {
        params += ", ";
      }
      params += fieldStorageType(f.kind);
      params += " ";
      params += f.name;
    }
    for (const NamedField &f : inheritedFields) {
      if (!params.empty()) {
        params += ", ";
      }
      params += fieldStorageType(f.kind);
      params += " ";
      params += f.name;
    }
    if (ownFields.empty() && inheritedFields.empty()) {
      fmt.linef("explicit {0}() : {1}(HirKind::{0}) {{}", name, parentName);
    } else {
      fmt.linef("{0}({1})", name, params);
      {
        auto inner = fmt.block();
        std::string init = parentName + "(HirKind::" + name;
        for (const NamedField &f : inheritedFields) {
          init += ", " + f.name;
        }
        init += ")";
        for (const NamedField &f : ownFields) {
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

    for (const NamedField &f : ownFields) {
      const std::string accessor = "get" + capitalize(f.name);
      const std::string type = fieldStorageType(f.kind);
      fmt.line("");
      fmt.linef("[[nodiscard]] {0} {1}() const {{ return {2}; }", type,
                accessor, f.name);
    }
  }

  if (!ownFields.empty()) {
    fmt.line("");
    fmt.line("private:");
    auto body = fmt.block();
    for (const NamedField &f : ownFields) {
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
