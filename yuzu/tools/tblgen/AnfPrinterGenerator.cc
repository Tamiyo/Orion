#include "AnfPrinterGenerator.h"

#include "utils/SchemaUtils.h"
#include "utils/StringUtils.h"
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

/// Sort defs by source position so the emitted output follows declaration
/// order, matching the layout used by every other generator.
bool byLoc(const llvm::Record *a, const llvm::Record *b) {
  return a->getLoc().front().getPointer() < b->getLoc().front().getPointer();
}

/// True when a field is the resolved type (`Custom<TypeRef>`), which is
/// rendered inline after the node name rather than as an ordinary field. The
/// type is declared once on the `Expr` base and inherited, so a concrete node
/// finds it among its inherited fields.
bool isTypeField(const NamedField &f) {
  if (const auto *custom = std::get_if<Custom>(&f.kind)) {
    return custom->typeName == "TypeRef";
  }
  return false;
}

/// Walk `record`'s `Parent` chain (excluding `record` itself), collecting each
/// level's `Fields`. Mirrors the node/builder generators' helper of the same
/// name; used here to find the inherited `Expr` type field.
std::vector<NamedField> gatherInheritedFields(const llvm::Record *record) {
  std::vector<NamedField> out;
  const llvm::Record *cur = record;
  while (cur->getValue("Parent")) {
    cur = cur->getValueAsDef("Parent");
    const auto fields = parseFields(cur);
    out.insert(out.end(), fields.begin(), fields.end());
  }
  return out;
}

/// Emit print code for a single (non-type) field. Every field begins with its
/// own `os << "\n"` so the caller needn't track preceding output — except
/// `Children` lists, which emit a `\n` per element (an empty list emits
/// nothing).
void emitField(CodeFormatter &fmt, const NamedField &f,
               const llvm::RecordKeeper &records) {
  const std::string getter = "node->get" + capitalize(f.name) + "()";

  std::visit(
      [&](const auto &k) {
        using T = std::decay_t<decltype(k)>;
        if constexpr (std::is_same_v<T, Child>) {
          // A `Child<T>` may be null (e.g. a bare `return`'s value), so guard.
          fmt.linef("if ({0} != nullptr) {{", getter);
          {
            auto body = fmt.block();
            fmt.line("os << \"\\n\";");
            fmt.linef("printNode(os, {0}, indent + 1);", getter);
          }
          fmt.line("}");
        } else if constexpr (std::is_same_v<T, Children>) {
          fmt.linef("for (const auto *child : {0}) {{", getter);
          {
            auto body = fmt.block();
            fmt.line("os << \"\\n\";");
            fmt.line("printNode(os, child, indent + 1);");
          }
          fmt.line("}");
        } else if constexpr (std::is_same_v<T, Custom> ||
                             std::is_same_v<T, Val>) {
          const llvm::Record *typeDef = records.getDef(k.typeName);
          if (!typeDef) {
            llvm::PrintFatalError("yuzu-tblgen: AnfPrinterGenerator: "
                                  "unknown type '" +
                                  k.typeName + "' for field $" + f.name);
          }
          fmt.line("os << \"\\n\";");
          fmt.line("os.indent((indent + 1) * 2);");

          if (typeDef->isSubClassOf("Enum")) {
            fmt.linef("os << \"{0}=\" << asString({1});", f.name, getter);
            return;
          }
          if (typeDef->isSubClassOf("Native")) {
            const std::string nativeName =
                typeDef->getValueAsString("Name").str();
            if (nativeName == "bool") {
              fmt.linef("os << \"{0}=\" << ({1} ? \"true\" : \"false\");",
                        f.name, getter);
              return;
            }
            if (nativeName == "std::u32string" ||
                nativeName == "std::u32string_view") {
              fmt.linef("os << \"{0}=\";", f.name);
              fmt.linef("util::writeUtf8(os, {0});", getter);
              return;
            }
            if (nativeName == "yuzu::BuiltinOp") {
              // A `BuiltinOp` is a value (an enum); print its stable name.
              fmt.linef("os << \"{0}=\" << name({1});", f.name, getter);
              return;
            }
            if (nativeName == "const yuzu::anf::Binding *") {
              // A back-edge to a `Binding`: print its name, not the pointer,
              // and don't recurse (the binding is owned and printed elsewhere).
              fmt.linef("if ({0} != nullptr) {{", getter);
              {
                auto body = fmt.block();
                fmt.linef("os << \"{0}=\";", f.name);
                fmt.linef("util::writeUtf8(os, {0}->getIdent()->getName());",
                          getter);
              }
              fmt.line("}");
              return;
            }
            if (nativeName == "const yuzu::anf::FuncStmt *") {
              // A reference to a `FuncStmt`: print its name, not the pointer,
              // and don't recurse (the function is owned by the root).
              fmt.linef("if ({0} != nullptr) {{", getter);
              {
                auto body = fmt.block();
                fmt.linef("os << \"{0}=\";", f.name);
                fmt.linef("util::writeUtf8(os, {0}->getName()->getName());",
                          getter);
              }
              fmt.line("}");
              return;
            }
            fmt.linef("os << \"{0}=\" << {1};", f.name, getter);
            return;
          }
          llvm::PrintFatalError("yuzu-tblgen: AnfPrinterGenerator: don't "
                                "know how to print Custom<" +
                                k.typeName +
                                "> — type is neither Native nor Enum");
        } else {
          llvm::PrintFatalError("yuzu-tblgen: AnfPrinterGenerator has no "
                                "emitter for this Field kind yet");
        }
      },
      f.kind);
}

/// Emit a `print<X>` for one concrete Node: the indented node name, the on-node
/// type inline (` : <type>`), then every own non-type field in declaration
/// order. No trailing newline.
void emitPrintNode(CodeFormatter &fmt, const llvm::Record *node,
                   const llvm::RecordKeeper &records) {
  const std::string name = node->getName().str();
  const std::vector<NamedField> fields = parseFields(node);

  fmt.linef("inline void print{0}(llvm::raw_ostream &os, const {0} *node, "
            "std::size_t indent) {{",
            name);
  {
    auto body = fmt.block();
    fmt.line("os.indent(indent * 2);");
    fmt.linef("os << \"{0}\";", name);

    // Single `Ident` children render inline as `field=name` on this line, so a
    // bare name doesn't cost its own nested node.
    for (const NamedField &f : fields) {
      if (!isChildOf(f, "Ident")) {
        continue;
      }
      const std::string getter = "node->get" + capitalize(f.name) + "()";
      fmt.linef("if ({0} != nullptr) {{", getter);
      {
        auto inner = fmt.block();
        fmt.linef("os << \" {0}=\";", f.name);
        fmt.linef("util::writeUtf8(os, {0}->getName());", getter);
      }
      fmt.line("}");
    }

    // The resolved type, printed inline after the name (skipped when unset).
    // It lives on the `Expr` base, so look through inherited fields too.
    std::vector<NamedField> typeSearch = fields;
    const std::vector<NamedField> inherited = gatherInheritedFields(node);
    typeSearch.insert(typeSearch.end(), inherited.begin(), inherited.end());
    for (const NamedField &f : typeSearch) {
      if (!isTypeField(f)) {
        continue;
      }
      const std::string getter = "node->get" + capitalize(f.name) + "()";
      fmt.linef("if ({0} != nullptr) {{", getter);
      fmt.linef("  os << \" : \" << yuzu::types::asString({0}->getKind());",
                getter);
      fmt.line("}");
      break;
    }

    for (const NamedField &f : fields) {
      if (!isTypeField(f) && !isChildOf(f, "Ident")) {
        emitField(fmt, f, records);
      }
    }
  }
  fmt.line("}");
  fmt.line("");
}

/// Emit the `printNode` dispatcher: switch over `<Tree>Kind` for every concrete
/// Node, calling its `print<X>`. Variants/sentinels can't appear on a real
/// node.
void emitDispatcher(CodeFormatter &fmt, llvm::StringRef treeName,
                    const std::vector<const llvm::Record *> &nodes) {
  fmt.linef("inline void printNode(llvm::raw_ostream &os, const {0}Node *node, "
            "std::size_t indent) {{",
            treeName);
  {
    auto body = fmt.block();
    fmt.linef("switch (node->getKind()) {{");
    for (const llvm::Record *n : nodes) {
      const std::string name = n->getName().str();
      fmt.linef("case {0}Kind::{1}:", treeName, name);
      fmt.linef("  return print{0}(os, {0}::cast(node), indent);", name);
    }
    fmt.line("default:");
    fmt.line("  util::yuzu_unreachable();");
    fmt.line("}");
  }
  fmt.line("}");
  fmt.line("");
}

} // namespace

void AnfPrinterGenerator::generate(const llvm::RecordKeeper &records) {
  const std::string ns = findNamespace(records, "Base");
  const llvm::StringRef treeName = findTreeName(records, "Base");

  std::vector<const llvm::Record *> nodes =
      records.getAllDerivedDefinitions("Node");
  std::sort(nodes.begin(), nodes.end(), byLoc);

  fmt.linef("namespace {0} {{", ns);
  fmt.line("");

  // Forward-declare `printNode` so per-node functions can recurse into
  // children.
  fmt.linef("inline void printNode(llvm::raw_ostream &os, const {0}Node *node, "
            "std::size_t indent);",
            treeName);
  fmt.line("");

  for (const llvm::Record *n : nodes) {
    emitPrintNode(fmt, n, records);
  }

  emitDispatcher(fmt, treeName, nodes);

  fmt.linef("} // namespace {0}", ns);
}

} // namespace yuzu::tools
