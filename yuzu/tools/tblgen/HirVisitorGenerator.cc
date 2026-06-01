#include "HirVisitorGenerator.h"

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

//===----------------------------------------------------------------------===//
// HirVisitor layout
//===----------------------------------------------------------------------===//
//
//   template <typename Derived>
//   class HirVisitor {
//   public:
//     void visit(const HirNode *node) {
//       switch (node->getKind()) {
//       case HirKind::BinaryExpr:
//         return derived().traverseBinaryExpr(BinaryExpr::cast(node));
//       ...
//       default:
//         util::yuzu_unreachable();
//       }
//     }
//
//     // Default traverse: walk children, then visit.
//     void traverseBinaryExpr(const BinaryExpr *node) {
//       derived().walkBinaryExpr(node);
//       derived().visitBinaryExpr(node);
//     }
//
//     // Default walk: recurse into each child.
//     void walkBinaryExpr(const BinaryExpr *node) {
//       derived().visit(node->getLhs());
//       derived().visit(node->getRhs());
//     }
//
//     // Hook: default no-op.
//     void visitBinaryExpr(const BinaryExpr *node) { (void)node; }
//
//   private:
//     Derived &derived() { return *static_cast<Derived *>(this); }
//   };

/// Sort defs by source position so the emitted output follows
/// declaration order. Matches the convention used by every other
/// generator.
bool byLoc(const llvm::Record *a, const llvm::Record *b) {
  return a->getLoc().front().getPointer() < b->getLoc().front().getPointer();
}

/// True if `f` is a structural child — one of the two Field shapes that
/// the walker recurses into (`Child<T>` and `Children<T>`). Custom /
/// Val / Native / Enum fields are data, not tree edges, and are left
/// for `visitX` hooks to inspect.
bool isStructuralChild(const NamedField &f) {
  return std::holds_alternative<Child>(f.kind) ||
         std::holds_alternative<Children>(f.kind);
}

/// Emit one line inside `walkX` for a single structural-child field:
/// a direct `visit(node->getF())` for `Child<T>` and a loop over the
/// list view for `Children<T>`. Non-structural fields are filtered out
/// before reaching this helper.
void emitChildWalk(CodeFormatter &fmt, const NamedField &f) {
  const std::string getter = "node->get" + capitalize(f.name) + "()";
  std::visit(
      [&](const auto &k) {
        using T = std::decay_t<decltype(k)>;
        if constexpr (std::is_same_v<T, Child>) {
          // A `Child<T>` may be null (an optional child like a bare
          // `return`'s expr), so guard before descending.
          fmt.linef("if ({0} != nullptr) {{", getter);
          {
            auto body = fmt.block();
            fmt.linef("derived().visit({0});", getter);
          }
          fmt.line("}");
        } else if constexpr (std::is_same_v<T, Children>) {
          fmt.linef("for (const auto *child : {0}) {{", getter);
          {
            auto body = fmt.block();
            fmt.line("derived().visit(child);");
          }
          fmt.line("}");
        } else {
          llvm::PrintFatalError(
              "yuzu-tblgen: HirVisitorGenerator: emitChildWalk called on a "
              "non-structural field — filter callers via isStructuralChild");
        }
      },
      f.kind);
}

/// Emit `traverseX` for one Node: by default walk children then visit.
/// Both calls go through `derived()` so subclasses can override either.
void emitTraverse(CodeFormatter &fmt, const llvm::Record *node) {
  const std::string name = node->getName().str();
  fmt.linef("void traverse{0}(const {0} *node) {{", name);
  {
    auto body = fmt.block();
    fmt.linef("derived().walk{0}(node);", name);
    fmt.linef("derived().visit{0}(node);", name);
  }
  fmt.line("}");
  fmt.line("");
}

/// Emit `walkX` for one Node: recurse into every structural-child
/// field. If the Node has no structural children the body collapses to
/// `(void)node;` so `-Wunused-parameter` stays clean.
void emitWalk(CodeFormatter &fmt, const llvm::Record *node) {
  const std::string name = node->getName().str();
  const std::vector<NamedField> fields = parseFields(node);

  fmt.linef("void walk{0}(const {0} *node) {{", name);
  {
    auto body = fmt.block();
    bool hasChild = false;
    for (const NamedField &f : fields) {
      if (!isStructuralChild(f)) {
        continue;
      }
      emitChildWalk(fmt, f);
      hasChild = true;
    }
    if (!hasChild) {
      // Silence -Wunused-parameter on leaf Nodes (literals, etc.).
      fmt.line("(void)node;");
    }
  }
  fmt.line("}");
  fmt.line("");
}

/// Emit `visitX` for one Node — the user hook. Default body is a no-op;
/// the `(void)node` silences -Wunused-parameter when the subclass
/// doesn't override.
void emitVisit(CodeFormatter &fmt, const llvm::Record *node) {
  const std::string name = node->getName().str();
  fmt.linef("void visit{0}(const {0} *node) {{ (void)node; }", name);
}

/// Emit the top-level `visit(const HirNode *)` dispatcher: a switch
/// over `HirKind` for every concrete Node, forwarding to its own
/// `traverseX` via `derived()`. Variants and System kinds can't appear
/// on a concrete node so they fall through to `yuzu_unreachable`.
void emitDispatcher(CodeFormatter &fmt,
                    const std::vector<const llvm::Record *> &nodes) {
  fmt.line("void visit(const HirNode *node) {");
  {
    auto body = fmt.block();
    fmt.line("switch (node->getKind()) {");
    for (const llvm::Record *n : nodes) {
      const std::string name = n->getName().str();
      fmt.linef("case HirKind::{0}:", name);
      fmt.linef("  return derived().traverse{0}({0}::cast(node));", name);
    }
    fmt.line("default:");
    fmt.line("  util::yuzu_unreachable();");
    fmt.line("}");
  }
  fmt.line("}");
  fmt.line("");
}

} // namespace

void HirVisitorGenerator::generate(const llvm::RecordKeeper &records) {
  const std::string ns = findNamespace(records, "Base");

  std::vector<const llvm::Record *> nodes =
      records.getAllDerivedDefinitions("Node");
  std::sort(nodes.begin(), nodes.end(), byLoc);

  fmt.linef("namespace {0} {{", ns);
  fmt.line("");

  fmt.line("template <typename Derived>");
  fmt.line("class HirVisitor {");
  fmt.line("public:");
  {
    auto pub = fmt.block();
    emitDispatcher(fmt, nodes);
  }
  fmt.line("");
  // Overridable hooks are protected: external code shouldn't call
  // `c.traverseFoo(...)` directly, only `c.visit(node)`. Subclasses
  // must override these as `public` (or `friend HirVisitor<Self>`) —
  // protected-on-protected breaks the base's `derived().visitX(...)`
  // call site under C++'s naming-class access rule.
  fmt.line("protected:");
  {
    auto prot = fmt.block();
    // Per-Node `traverseX`. Emitted before walk/visit so the call sites
    // forward through `derived()` consistently — order between the
    // three blocks is purely cosmetic, the template only resolves them
    // at instantiation.
    for (const llvm::Record *n : nodes) {
      emitTraverse(fmt, n);
    }

    for (const llvm::Record *n : nodes) {
      emitWalk(fmt, n);
    }

    for (const llvm::Record *n : nodes) {
      emitVisit(fmt, n);
    }
  }
  fmt.line("");
  fmt.line("private:");
  {
    auto priv = fmt.block();
    fmt.line("Derived &derived() { return *static_cast<Derived *>(this); }");
  }
  fmt.line("};");
  fmt.line("");

  fmt.linef("} // namespace {0}", ns);
}

} // namespace yuzu::tools
