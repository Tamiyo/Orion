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
          // A `Child<T>` may be null (an optional child like a bare
          // `return`'s expr), so guard before printing it.
          fmt.linef("if ({0} != nullptr) {{", getter);
          {
            auto body = fmt.block();
            fmt.line("os << \"\\n\";");
            fmt.linef("printNode(os, ctx, {0}, indent + 1);", getter);
          }
          fmt.line("}");
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

/// Emit the inline annotations that follow a node's name on the same
/// line:
///   ` : <type>`   — reads the type side table (`ctx.getTypeContext()`).
///   ` → <kind> <target>` — reads the adjustment side table.
/// Both are skipped silently when the node has no entry, so the printer
/// remains useful at any pipeline stage.
void emitAnnotations(CodeFormatter &fmt) {
  fmt.line("if (const auto *t = ctx.getTypeContext().typeOf(node)) {");
  fmt.line("  os << \" : \" << asString(t->getKind());");
  fmt.line("}");
  fmt.line("if (const auto a = ctx.getAdjustments().get(node->getId())) {");
  fmt.line("  os << \" \\xE2\\x86\\x92 \" << asString(a->kind) << ' '");
  fmt.line("     << asString(a->target->getKind());");
  fmt.line("}");
}

/// Emit a `print<X>` function for one concrete Node: the indented node
/// name (no trailing newline), inline annotations (type and adjustment,
/// both read from `ctx`'s side tables), then every own field in
/// declaration order. Inherited fields aren't printed (the implicit `id`
/// and the side-table type carry no schema field).
void emitPrintNode(CodeFormatter &fmt, const llvm::Record *node,
                   const llvm::RecordKeeper &records) {
  const std::string name = node->getName().str();
  const std::vector<NamedField> fields = parseFields(node);

  fmt.linef("inline void print{0}(llvm::raw_ostream &os, "
            "HirContext &ctx, const {0} *node, "
            "std::size_t indent) {{",
            name);
  {
    auto body = fmt.block();
    fmt.line("os.indent(indent * 2);");
    fmt.linef("os << \"{0}\";", name);
    emitAnnotations(fmt);
    for (const NamedField &f : fields) {
      emitField(fmt, f, records);
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
