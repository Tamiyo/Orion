#include "HirPrinterGenerator.h"

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

/// Sort defs by source position so the emitted output follows
/// declaration order. Keeps the printer's output deterministic and
/// matches the layout used by every other generator.
bool byLoc(const llvm::Record *a, const llvm::Record *b) {
  return a->getLoc().front().getPointer() < b->getLoc().front().getPointer();
}

/// Emit print code for a single field. Every field begins with its own
/// `os << "\n"` so the calling frame doesn't need to track whether
/// preceding fields emitted output — except for `Children` lists, which
/// emit a `\n` per element inside their loop (an empty list emits
/// nothing, naturally collapsing to whatever the next field does).
void emitField(CodeFormatter &fmt, const NamedField &f,
               const llvm::RecordKeeper &records) {
  const std::string getter = "node->get" + capitalize(f.name) + "()";

  std::visit(
      [&](const auto &k) {
        using T = std::decay_t<decltype(k)>;
        if constexpr (std::is_same_v<T, Child>) {
          fmt.line("os << \"\\n\";");
          fmt.linef("printNode(os, ctx, {0}, indent + 1);", getter);
        } else if constexpr (std::is_same_v<T, Children>) {
          fmt.linef("for (const auto *child : {0}) {{", getter);
          {
            auto body = fmt.block();
            fmt.line("os << \"\\n\";");
            fmt.line("printNode(os, ctx, child, indent + 1);");
          }
          fmt.line("}");
        } else if constexpr (std::is_same_v<T, Custom> ||
                             std::is_same_v<T, Val>) {
          const llvm::Record *typeDef = records.getDef(k.typeName);
          if (!typeDef) {
            llvm::PrintFatalError("yuzu-tblgen: HirPrinterGenerator: "
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
            if (nativeName == "std::u32string") {
              fmt.linef("os << \"{0}=\";", f.name);
              fmt.linef("util::writeUtf8(os, {0});", getter);
              return;
            }
            if (nativeName == "const yuzu::hir::Op *") {
              fmt.linef("os << \"{0}=\" << {1}->getName();", f.name, getter);
              return;
            }
            fmt.linef("os << \"{0}=\" << {1};", f.name, getter);
            return;
          }
          llvm::PrintFatalError("yuzu-tblgen: HirPrinterGenerator: don't "
                                "know how to print Custom<" +
                                k.typeName +
                                "> — type is neither Native "
                                "nor Enum");
        } else {
          llvm::PrintFatalError("yuzu-tblgen: HirPrinterGenerator has no "
                                "emitter for this Field kind yet");
        }
      },
      f.kind);
}

/// Walk `record`'s parent chain (excluding itself) and return every
/// inherited Field, innermost-to-outermost. Lets the printer detect
/// whether a Node inherits a `type` field (the convention used by
/// `Expr`-derived Nodes) so it can emit the inline `: <type>`
/// annotation reading directly from `node->getType()`.
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

/// True if the node inherits a `Custom<TypeRef>:$type` field — by
/// convention, this means the node is `Expr`-derived and has a
/// printable inline type annotation.
bool hasInheritedType(const std::vector<NamedField> &inheritedFields) {
  for (const NamedField &f : inheritedFields) {
    if (f.name != "type") {
      continue;
    }
    if (const auto *c = std::get_if<Custom>(&f.kind)) {
      if (c->typeName == "TypeRef") {
        return true;
      }
    }
  }
  return false;
}

/// Emit the inline annotations that follow a node's name on the same
/// line:
///   ` : <type>`   — reads `node->getType()` for `Expr`-derived nodes
///                   (those that inherit `Custom<TypeRef>:$type`).
///   ` → <kind> <target>` — reads the adjustment side table.
/// Both are skipped silently when not applicable, so the printer
/// remains useful at any pipeline stage.
void emitAnnotations(CodeFormatter &fmt, bool hasType) {
  if (hasType) {
    fmt.line("os << \" : \" << asString(node->getType()->getKind());");
  }
  fmt.line("if (const auto a = ctx.getAdjustments().get(node->getId())) {");
  fmt.line("  os << \" \\xE2\\x86\\x92 \" << asString(a->kind) << ' '");
  fmt.line("     << asString(a->target->getKind());");
  fmt.line("}");
}

/// Emit a `print<X>` function for one concrete Node: the indented node
/// name (no trailing newline), inline annotations (type from
/// `node->getType()` when present, adjustment from the side table),
/// then every own field in declaration order. Inherited fields aren't
/// printed (they're either the inline-annotated `type` or the implicit
/// `id`).
void emitPrintNode(CodeFormatter &fmt, const llvm::Record *node,
                   const llvm::RecordKeeper &records) {
  const std::string name = node->getName().str();
  const std::vector<NamedField> fields = parseFields(node);
  const std::vector<NamedField> inheritedFields = gatherInheritedFields(node);
  const bool hasType = hasInheritedType(inheritedFields);

  fmt.linef("inline void print{0}(llvm::raw_ostream &os, "
            "HirContext &ctx, const {0} *node, "
            "std::size_t indent) {{",
            name);
  {
    auto body = fmt.block();
    fmt.line("os.indent(indent * 2);");
    fmt.linef("os << \"{0}\";", name);
    emitAnnotations(fmt, hasType);
    for (const NamedField &f : fields) {
      emitField(fmt, f, records);
    }
    if (fields.empty() && !hasType) {
      // Silence -Wunused-parameter when a Node has nothing to print
      // beyond its name. `node` and `indent` aren't referenced otherwise.
      fmt.line("(void)node;");
    }
  }
  fmt.line("}");
  fmt.line("");
}

/// Emit the `printNode` dispatcher: switch over `HirKind` for every
/// concrete Node, calling its specific `print<X>`. Variant kinds and
/// sentinels can't appear on a real node, so they fall through to
/// `yuzu_unreachable`.
void emitDispatcher(CodeFormatter &fmt,
                    const std::vector<const llvm::Record *> &nodes) {
  fmt.line("inline void printNode(llvm::raw_ostream &os, "
           "HirContext &ctx, const HirNode *node, "
           "std::size_t indent) {");
  {
    auto body = fmt.block();
    fmt.line("switch (node->getKind()) {");
    for (const llvm::Record *n : nodes) {
      const std::string name = n->getName().str();
      fmt.linef("case HirKind::{0}:", name);
      fmt.linef("  return print{0}(os, ctx, {0}::cast(node), indent);", name);
    }
    fmt.line("default:");
    fmt.line("  util::yuzu_unreachable();");
    fmt.line("}");
  }
  fmt.line("}");
  fmt.line("");
}

} // namespace

void HirPrinterGenerator::generate(const llvm::RecordKeeper &records) {
  const std::string ns = findNamespace(records, "Base");

  std::vector<const llvm::Record *> nodes =
      records.getAllDerivedDefinitions("Node");
  std::sort(nodes.begin(), nodes.end(), byLoc);

  fmt.linef("namespace {0} {{", ns);
  fmt.line("");

  // Forward-declare `printNode` so per-node functions can call it for
  // child recursion (a node's children include the node itself when
  // there's nesting in the schema).
  fmt.line("inline void printNode(llvm::raw_ostream &os, "
           "HirContext &ctx, const HirNode *node, "
           "std::size_t indent);");
  fmt.line("");

  for (const llvm::Record *n : nodes) {
    emitPrintNode(fmt, n, records);
  }

  emitDispatcher(fmt, nodes);

  fmt.linef("} // namespace {0}", ns);
}

} // namespace yuzu::tools
