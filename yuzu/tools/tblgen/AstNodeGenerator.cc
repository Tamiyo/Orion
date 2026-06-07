#include "AstNodeGenerator.h"

#include "utils/EnumEmitter.h"
#include "utils/SchemaUtils.h"
#include "utils/StringUtils.h"
#include "utils/TreeUtils.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/TableGen/Error.h>
#include <llvm/TableGen/Record.h>

#include <algorithm>
#include <map>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace yuzu::tools {

namespace {

/// Sort defs by source position so emitted classes follow `.td` declaration
/// order — a node's inline accessor constructs its child types, which must
/// be defined first. `getAllDerivedDefinitions` returns them alphabetically.
bool byLoc(const llvm::Record *a, const llvm::Record *b) {
  return a->getLoc().front().getPointer() < b->getLoc().front().getPointer();
}

//===----------------------------------------------------------------------===//
// Class definitions
//===----------------------------------------------------------------------===//
//
// Each emitted AST type is a POD view over a `SyntaxNode`, e.g.:
//
//   class AstNode {
//   public:
//     SyntaxKind getKind() const;
//     const SyntaxNode &getSyntax() const;
//   protected:
//     explicit AstNode(SyntaxNode node);
//     SyntaxNode node;
//   };
//
//   class Foo : public AstNode {
//   public:
//     explicit Foo(SyntaxNode node);
//     static bool isA(SyntaxKind);
//     static std::optional<Foo> cast(const AstNode &);
//     ... accessor declarations ...
//   };

/// Find the unique grammar root: a `Base`-derived def that is *not* a
/// `Variant`. Variants subclass `Base` in the schema, so we filter them out.
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

/// llvm::formatv quirk: `{{` is an escape for a literal `{`, but `}` is
/// literal on its own. So `{{}` produces `{}`, *not* `{{}}`. Using `}}`
/// would emit a stray `}` and silently corrupt class structure.

/// Emit the grammar root: stores the `SyntaxNode`, exposes `getKind`,
/// `getSyntax`, and `getRange`, and provides a `protected` ctor for
/// subclasses.
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
    fmt.line("[[nodiscard]] SyntaxKind getKind() const "
             "{ return node.getKind(); }");
    fmt.line("");
    fmt.line("[[nodiscard]] const SyntaxNode &getSyntax() const "
             "{ return node; }");
    fmt.line("");
    fmt.line("[[nodiscard]] lexer::Range getRange() const {");
    {
      auto inner = fmt.block();
      fmt.line("const auto start = static_cast<uint32_t>(node.getOffset());");
      fmt.line("const auto end = start "
               "+ static_cast<uint32_t>(node.getGreen().getWidth());");
      fmt.line("return lexer::Range{start, end};");
    }
    fmt.line("}");
  }
  fmt.line("");
  fmt.line("protected:");
  {
    auto body = fmt.block();
    fmt.linef("explicit {0}(SyntaxNode node) : node(std::move(node)) {{}",
              name);
    fmt.linef("{}() = delete;", name);
    fmt.line("");
    fmt.line("SyntaxNode node;");
  }
  fmt.line("};");
  fmt.line("");
}

/// Emit a Variant class. `isA` is a `SyntaxKind` range check between the
/// `<V>_FIRST` / `<V>_LAST` sentinels.
void emitVariantClass(CodeFormatter &fmt, const llvm::Record *variant,
                      llvm::StringRef baseName) {
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
    fmt.linef("explicit {0}(SyntaxNode node) : {1}(std::move(node)) {{}", name,
              parentName);
    fmt.linef("{}() = delete;", name);
    fmt.line("");

    fmt.line("[[nodiscard]] static bool isA(SyntaxKind kind) {");
    {
      auto inner = fmt.block();
      fmt.linef("return kind > SyntaxKind::{0}_FIRST "
                "&& kind < SyntaxKind::{0}_LAST;",
                sentinel);
    }
    fmt.line("}");
    fmt.line("");

    fmt.linef("[[nodiscard]] static std::optional<{0}> "
              "cast(const {1} &node) {{",
              name, baseName);
    {
      auto inner = fmt.block();
      fmt.line("if (!isA(node.getKind())) {");
      fmt.line("  return std::nullopt;");
      fmt.line("}");
      fmt.linef("return {0}(node.getSyntax());", name);
    }
    fmt.line("}");
    fmt.line("");

    // Narrow kind for one-level dispatch over this variant's direct children.
    fmt.linef("[[nodiscard]] {0}Kind get{0}Kind() const "
              "{{ return to{0}Kind(getKind()); }",
              name);
  }
  fmt.line("};");
  fmt.line("");
}

void emitNodeClass(CodeFormatter &fmt, const llvm::Record *node,
                   llvm::StringRef baseName) {
  const std::string name = node->getName().str();
  const std::string parentName = node->getValueAsDef("Parent")->getName().str();
  const llvm::StringRef summary = node->getValueAsString("Summary");

  if (!summary.empty()) {
    fmt.linef("/// {0}", summary);
  }
  fmt.linef("class [[nodiscard]] {0} : public {1} {{", name, parentName);
  fmt.line("public:");
  {
    auto body = fmt.block();
    fmt.linef("explicit {0}(SyntaxNode node) : {1}(std::move(node)) {{}", name,
              parentName);
    fmt.linef("{}() = delete;", name);
    fmt.line("");

    fmt.linef("[[nodiscard]] static bool isA(SyntaxKind kind) "
              "{{ return kind == SyntaxKind::{0}; }",
              name);
    fmt.line("");
    fmt.linef("[[nodiscard]] static std::optional<{0}> "
              "cast(const {1} &node) {{",
              name, baseName);
    {
      auto inner = fmt.block();
      fmt.line("if (!isA(node.getKind())) {");
      fmt.line("  return std::nullopt;");
      fmt.line("}");
      fmt.linef("return {0}(node.getSyntax());", name);
    }
    fmt.line("}");

    // Accessors are emitted inline. All `Variant` types they reference are
    // already complete (Variants are emitted before any Node), so inline
    // definitions compile cleanly without out-of-line forwarding. `Custom`
    // accessors stay declaration-only — the implementation is hand-written.
    //
    // Per-type counter for `child<T>(node, n)` indexing — two
    // `Child<Expr>:$lhs/$rhs` pick up n=0 and n=1 respectively.
    std::map<std::string, int> childCounts;
    for (const NamedField &f : parseFields(node)) {
      const std::string accessor = "get" + capitalize(f.name);
      std::visit(
          [&](const auto &kind) {
            using T = std::decay_t<decltype(kind)>;
            if constexpr (std::is_same_v<T, Child>) {
              const int n = childCounts[kind.typeName]++;
              fmt.line("");
              fmt.linef("[[nodiscard]] std::optional<{0}> {1}() const {{",
                        kind.typeName, accessor);
              {
                auto inner = fmt.block();
                fmt.linef("return child<{0}>(node, {1});", kind.typeName, n);
              }
              fmt.line("}");
            } else if constexpr (std::is_same_v<T, Children>) {
              // `Children<T>:$f` is a lazy view over all matching children,
              // returned as `AstChildren<T>` (defined in Ast.h). The class
              // template already carries `[[nodiscard]]`, so functions
              // returning it by value are diagnosed without a function-level
              // attribute. Multiple `Children<T>` fields on the same node
              // would currently both alias the same view — there's no second
              // axis to distinguish them — so we don't bump childCounts here.
              fmt.line("");
              fmt.linef("AstChildren<{0}> {1}() const {{", kind.typeName,
                        accessor);
              {
                auto inner = fmt.block();
                fmt.linef("return AstChildren<{0}>(node.getChildren());",
                          kind.typeName);
              }
              fmt.line("}");
            } else if constexpr (std::is_same_v<T, Custom> ||
                                 std::is_same_v<T, Val>) {
              // Declaration-only accessor returning `std::optional<T>`. The
              // AST has no native storage for these — the implementation
              // is hand-written and typically reads from the underlying
              // syntax token. `accessorTypeName` picks the view alias
              // (e.g. `StringView`) when one is available, so callers can
              // avoid copying owned values out of the syntax tree.
              fmt.line("");
              fmt.linef("[[nodiscard]] std::optional<{0}> {1}() const;",
                        kind.accessorTypeName, accessor);
            } else {
              llvm::PrintFatalError(
                  "yuzu-tblgen: AstNodeGenerator has no accessor emitter for "
                  "this Field kind yet");
            }
          },
          f.kind);
    }
  }
  fmt.line("};");
  fmt.line("");
}

} // namespace

void AstNodeGenerator::generate(const llvm::RecordKeeper &records) {
  const std::string ns = findNamespace(records, "Base");
  const llvm::Record *base = findBase(records);

  std::vector<const llvm::Record *> variants =
      records.getAllDerivedDefinitions("Variant");
  std::vector<const llvm::Record *> nodes =
      records.getAllDerivedDefinitions("Node");
  std::sort(variants.begin(), variants.end(), byLoc);
  std::sort(nodes.begin(), nodes.end(), byLoc);

  fmt.linef("namespace {0} {{", ns);
  fmt.line("");

  // Native aliases and enums first — `Val<NativeDef>` and `Custom<...>`
  // accessors need them complete when the class that owns them is parsed.
  emitNatives(fmt, records);
  emitEnums(fmt, records);

  // Grammar root next: every Variant/Node inherits (transitively) from it,
  // so it must be complete before any subclass definition is parsed.
  emitBaseClass(fmt, base);

  // Forward-declare every Variant and Node so accessor signatures can name
  // them in either order.
  for (const llvm::Record *v : variants) {
    fmt.linef("class {0};", v->getName().str());
  }
  for (const llvm::Record *n : nodes) {
    fmt.linef("class {0};", n->getName().str());
  }
  if (!variants.empty() || !nodes.empty()) {
    fmt.line("");
  }

  const llvm::StringRef baseName = base->getName();

  // Variants first: their definitions only reference their `.td` Parent,
  // which is either the root Base or another Variant emitted earlier.
  for (const llvm::Record *v : variants) {
    emitVariantClass(fmt, v, baseName);
  }

  // Concrete nodes: accessor signatures and inline bodies reference Variant
  // types, which are complete. Each Node inherits from its `.td` Parent —
  // typically a Variant.
  for (const llvm::Record *n : nodes) {
    emitNodeClass(fmt, n, baseName);
  }

  fmt.linef("} // namespace {0}", ns);
}

} // namespace yuzu::tools
