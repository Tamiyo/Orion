#include "AstNodeGenerator.h"

#include "utils/SchemaUtils.h"
#include "utils/StringUtils.h"
#include "utils/TreeUtils.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/TableGen/Error.h>
#include <llvm/TableGen/Record.h>

#include <map>
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
// Each AST type is a POD-shaped view over `syntax::SyntaxNode`:
//
//   class Foo {
//   public:
//     explicit Foo(syntax::SyntaxNode n);
//     [[nodiscard]] static bool canCast(SyntaxKind);
//     [[nodiscard]] static std::optional<Foo> cast(syntax::SyntaxNode);
//     ... accessor declarations ...
//   private:
//     syntax::SyntaxNode node;
//   };
//
// SyntaxNode is itself a refcounted handle, so passing it by value is cheap
// (one atomic refcount op, no heap allocation). `cast` returns the view by
// value via `std::optional<T>` — no `unique_ptr`, no allocation.

/// Variant `canCast` does a SyntaxKind range check using the
/// `<V>_FIRST`/`<V>_LAST` sentinels emitted by `SyntaxKindGenerator`. No
/// `std::variant` — discriminating between concrete subtypes is a separate
/// `cast()` call to the concrete type.
void emitVariantClass(CodeFormatter &fmt, const llvm::Record *variant) {
  const std::string name = variant->getName().str();
  const llvm::StringRef summary = variant->getValueAsString("Summary");
  const std::string sentinel = llvm::StringRef(name).upper();

  if (!summary.empty()) {
    fmt.linef("/// {0}", summary);
  }
  fmt.linef("class [[nodiscard]] {0} {{", name);
  fmt.line("public:");
  {
    auto body = fmt.block();
    fmt.linef("explicit {0}(syntax::SyntaxNode n) : node(std::move(n)) {{}}",
              name);
    fmt.line("");

    fmt.line("[[nodiscard]] static bool canCast(SyntaxKind kind) {");
    {
      auto inner = fmt.block();
      fmt.linef("return kind > SyntaxKind::{0}_FIRST "
                "&& kind < SyntaxKind::{0}_LAST;",
                sentinel);
    }
    fmt.line("}");
    fmt.line("");

    fmt.linef("[[nodiscard]] static std::optional<{0}> "
              "cast(syntax::SyntaxNode n) {{",
              name);
    {
      auto inner = fmt.block();
      fmt.line("if (!canCast(static_cast<SyntaxKind>(n.getKind())))");
      fmt.line("  return std::nullopt;");
      fmt.linef("return {0}(std::move(n));", name);
    }
    fmt.line("}");
  }
  fmt.line("");
  fmt.line("private:");
  {
    auto body = fmt.block();
    fmt.line("syntax::SyntaxNode node;");
  }
  fmt.line("};");
  fmt.line("");
}

void emitNodeClass(CodeFormatter &fmt, const llvm::Record *node) {
  const std::string name = node->getName().str();
  const llvm::StringRef summary = node->getValueAsString("Summary");

  if (!summary.empty()) {
    fmt.linef("/// {0}", summary);
  }
  fmt.linef("class [[nodiscard]] {0} {{", name);
  fmt.line("public:");
  {
    auto body = fmt.block();
    fmt.linef("explicit {0}(syntax::SyntaxNode n) : node(std::move(n)) {{}}",
              name);
    fmt.line("");

    fmt.linef("[[nodiscard]] static bool canCast(SyntaxKind kind) "
              "{{ return kind == SyntaxKind::{0}; }}",
              name);
    fmt.line("");
    fmt.linef("[[nodiscard]] static std::optional<{0}> "
              "cast(syntax::SyntaxNode n) {{",
              name);
    {
      auto inner = fmt.block();
      fmt.line("if (!canCast(static_cast<SyntaxKind>(n.getKind())))");
      fmt.line("  return std::nullopt;");
      fmt.linef("return {0}(std::move(n));", name);
    }
    fmt.line("}");

    // Accessor *declarations* only — definitions land at file scope after
    // every class is complete (see emitAccessorDefs). This lets accessor
    // signatures reference variant types declared anywhere above.
    for (const NamedField &f : parseFields(node)) {
      const std::string accessor = "get" + capitalize(f.name);
      std::visit(
          [&](const auto &kind) {
            using T = std::decay_t<decltype(kind)>;
            if constexpr (std::is_same_v<T, Child>) {
              fmt.line("");
              fmt.linef("[[nodiscard]] std::optional<{0}> {1}() const;",
                        kind.typeName, accessor);
            } else if constexpr (std::is_same_v<T, Custom>) {
              // `Custom<T>:$f` is a declaration-only accessor returning `T`.
              // The implementation is hand-written elsewhere — generator
              // doesn't know how to derive the value.
              fmt.line("");
              fmt.linef("[[nodiscard]] std::optional<{0}> {1}() const;",
                        kind.typeName, accessor);
            } else {
              llvm::PrintFatalError(
                  "yuzu-tblgen: AstNodeGenerator has no accessor emitter for "
                  "this Field kind yet");
            }
          },
          f.kind);
    }
  }
  fmt.line("");
  fmt.line("private:");
  {
    auto body = fmt.block();
    fmt.line("syntax::SyntaxNode node;");
  }
  fmt.line("};");
  fmt.line("");
}

void emitAccessorDefs(CodeFormatter &fmt, const llvm::Record *node) {
  const std::string name = node->getName().str();

  // Per-type counter for `childOfType<T>(node, n)` indexing — two
  // `Child<Expr>:$lhs/$rhs` pick up n=0 and n=1 respectively.
  std::map<std::string, int> childCounts;
  for (const NamedField &f : parseFields(node)) {
    const std::string accessor = "get" + capitalize(f.name);
    std::visit(
        [&](const auto &kind) {
          using T = std::decay_t<decltype(kind)>;
          if constexpr (std::is_same_v<T, Child>) {
            const int n = childCounts[kind.typeName]++;
            fmt.linef(
                "inline std::optional<{0}> {1}::{2}() const {{",
                kind.typeName, name, accessor);
            {
              auto body = fmt.block();
              fmt.linef("return child<{0}>(node, {1});", kind.typeName, n);
            }
            fmt.line("}");
            fmt.line("");
          }
        },
        f.kind);
  }
}

/// Emit each `Enum` def as a C++ `enum class`. Ordered before the struct
/// definitions so `Custom<EnumDef>:$f` accessors can name the type.
void emitEnums(CodeFormatter &fmt, const llvm::RecordKeeper &records) {
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
  }
}

} // namespace

void AstNodeGenerator::generate(const llvm::RecordKeeper &records) {
  const std::string ns = findNamespace(records, "Base");

  const std::vector<const llvm::Record *> variants =
      records.getAllDerivedDefinitions("Variant");
  const std::vector<const llvm::Record *> nodes =
      records.getAllDerivedDefinitions("Node");

  fmt.linef("namespace {0} {{", ns);
  fmt.line("");

  // Enums first — `Custom<EnumDef>:$f` accessors need them to be a complete
  // type when the struct that owns them is parsed.
  emitEnums(fmt, records);

  // Forward-declare every struct so accessor signatures can name them in
  // either order.
  for (const llvm::Record *v : variants) {
    fmt.linef("struct {0};", v->getName().str());
  }
  for (const llvm::Record *n : nodes) {
    fmt.linef("struct {0};", n->getName().str());
  }
  if (!variants.empty() || !nodes.empty()) {
    fmt.line("");
  }

  // Variants first: their definitions don't reference any other AST type.
  for (const llvm::Record *v : variants) {
    emitVariantClass(fmt, v);
  }

  // Concrete nodes: accessor *declarations* may reference variant types,
  // which are now complete.
  for (const llvm::Record *n : nodes) {
    emitNodeClass(fmt, n);
  }

  // Out-of-line accessor definitions: every struct is complete by now, so
  // `childOfType<Variant>(...)` instantiations have everything they need.
  for (const llvm::Record *n : nodes) {
    emitAccessorDefs(fmt, n);
  }

  fmt.linef("} // namespace {0}", ns);
}

} // namespace yuzu::tools
