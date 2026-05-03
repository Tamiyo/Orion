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

void emitNodeClass(CodeFormatter &fmt, const llvm::Record *node) {
  const std::string name = node->getName().str();
  const llvm::StringRef summary = node->getValueAsString("Summary");

  if (!summary.empty()) {
    fmt.linef("/// {0}", summary);
  }
  
  fmt.linef("class {0} final : public AstNode<{0}> {{", name);
  {
    auto body = fmt.block();
    fmt.line("public:");
    fmt.linef(
        "explicit {0}(syntax::SyntaxNode node) : AstNode(std::move(node)) {{}",
        name);
    fmt.linef("{0}() = delete;", name);
    fmt.line("");

    fmt.linef("[[nodiscard]] static bool canCast(SyntaxKind kind) "
              "{{ return kind == SyntaxKind::{0}; }",
              name);
    fmt.linef("[[nodiscard]] static std::optional<{0}> "
              "cast(syntax::SyntaxNode node) {{",
              name);
    {
      auto cast = fmt.block();
      fmt.line("if (canCast(static_cast<SyntaxKind>(node.getKind()))) {");
      {
        auto branch = fmt.block();
        fmt.linef("return std::make_optional<{0}>(std::move(node));", name);
      }
      fmt.line("}");
      fmt.line("return std::nullopt;");
    }
    fmt.line("}");

    const std::vector<NamedField> fields = parseFields(node);
    if (!fields.empty()) {
      fmt.line("");
    }

    // For each Child<T>, runtime `child<T>(node, n)` counts by type — so two
    // `Child<Expr>:$lhs/$rhs` need n=0 and n=1 respectively. Track per-type
    // counts here to pick the right `n`.
    std::map<std::string, int> childCounts;
    for (const NamedField &f : fields) {
      const std::string accessor = "get" + capitalize(f.name);
      std::visit(
          [&](const auto &kind) {
            using T = std::decay_t<decltype(kind)>;
            if constexpr (std::is_same_v<T, Child>) {
              const int n = childCounts[kind.typeName]++;
              fmt.linef("[[nodiscard]] std::unique_ptr<{0}> {1}() const {{",
                        kind.typeName, accessor);
              {
                auto inner = fmt.block();
                fmt.linef("return child<{0}>(this->node, {1});",
                          kind.typeName, n);
              }
              fmt.line("}");
            } else {
              // Native / Enum / Val don't have an accessor shape yet —
              // adding one is a separate design conversation.
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

void emitVariantClass(CodeFormatter &fmt, const llvm::Record *variant,
                      const std::vector<const llvm::Record *> &children) {
  const std::string name = variant->getName().str();
  const llvm::StringRef summary = variant->getValueAsString("Summary");

  if (children.empty()) {
    // `std::variant<>` is ill-formed. Emit a placeholder so a forward
    // declaration backed by no concrete subtypes still resolves to a real
    // type.
    if (!summary.empty()) {
      fmt.linef("/// {0}", summary);
    }
    fmt.line("/// (no concrete subtypes yet)");
    fmt.linef("class {0} {{};", name);
    fmt.line("");
    return;
  }

  std::string list;
  for (size_t i = 0; i < children.size(); ++i) {
    if (i > 0)
      list += ", ";
    list += children[i]->getName().str();
  }

  if (!summary.empty()) {
    fmt.linef("/// {0}", summary);
  }
  fmt.linef("class {0} : public std::variant<{1}> {{", name, list);
  {
    auto body = fmt.block();
    fmt.line("public:");
    fmt.linef("using std::variant<{0}>::variant;", list);
    fmt.linef("{0}() = delete;", name);
    fmt.line("");

    std::string conds;
    for (size_t i = 0; i < children.size(); ++i) {
      if (i > 0)
        conds += " || ";
      conds += children[i]->getName().str() + "::canCast(kind)";
    }
    fmt.linef("[[nodiscard]] static bool canCast(SyntaxKind kind) "
              "{{ return {0}; }",
              conds);

    fmt.linef("[[nodiscard]] static std::optional<{0}> "
              "cast(syntax::SyntaxNode node) {{",
              name);
    {
      auto cast = fmt.block();
      fmt.line(
          "const SyntaxKind kind = static_cast<SyntaxKind>(node.getKind());");
      for (const llvm::Record *c : children) {
        const std::string cname = c->getName().str();
        fmt.linef("if ({0}::canCast(kind)) {{", cname);
        {
          auto branch = fmt.block();
          fmt.linef("return std::make_optional<{0}>({1}(std::move(node)));",
                    name, cname);
        }
        fmt.line("}");
      }
      fmt.line("return std::nullopt;");
    }
    fmt.line("}");
  }
  fmt.line("};");
  fmt.line("");
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

  // Forward-declare every Variant so concrete Node accessor return types
  // (`std::unique_ptr<Variant>`) compile before the Variant is fully defined.
  for (const llvm::Record *v : variants) {
    fmt.linef("class {0};", v->getName().str());
  }
  if (!variants.empty())
    fmt.line("");

  // Concrete Nodes first — Variants below need them as complete types.
  for (const llvm::Record *n : nodes) {
    emitNodeClass(fmt, n);
  }

  // Variants aggregate their concrete children.
  for (const llvm::Record *v : variants) {
    std::vector<const llvm::Record *> children;
    for (const llvm::Record *n : nodes) {
      if (n->getValueAsDef("Parent") == v) {
        children.push_back(n);
      }
    }
    emitVariantClass(fmt, v, children);
  }

  fmt.linef("} // namespace {0}", ns);
}

} // namespace yuzu::tools
