#include "HirKindGenerator.h"

#include "utils/SchemaUtils.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/TableGen/Record.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace yuzu::tools {

namespace {

//===----------------------------------------------------------------------===//
// Helpers
//===----------------------------------------------------------------------===//

/// Smallest unsigned integer type that fits `count` distinct values.
llvm::StringRef getUnderlyingType(std::size_t count) {
  if (count <= std::numeric_limits<std::uint8_t>::max()) {
    return "uint8_t";
  }

  if (count <= std::numeric_limits<std::uint16_t>::max()) {
    return "uint16_t";
  }

  if (count <= std::numeric_limits<std::uint32_t>::max()) {
    return "uint32_t";
  }

  return "uint64_t";
}

/// Sort defs by source position so the emitted enum follows include/file
/// order. Matches `SyntaxKindGenerator::byLoc` — kept identical so the
/// two generators don't drift apart on ordering rules.
bool byLoc(const llvm::Record *a, const llvm::Record *b) {
  return a->getLoc().front().getPointer() < b->getLoc().front().getPointer();
}

//===----------------------------------------------------------------------===//
// Variant tree
//===----------------------------------------------------------------------===//
//
// Both the master `HirKind` enum and the per-variant `<V>Kind` enums need
// to walk the same parent→children structure. Rather than re-query
// `(variants, nodes)` for `Parent` matches at every emitter, build the
// tree once and have every emitter walk it.

/// One node in the variant tree: either a concrete schema `Node` leaf
/// (`isVariant == false`, `children` empty) or a `Variant` with its own
/// children. `children` is sorted by source location so the layout
/// matches declaration order in the `.td`.
struct Member {
  const llvm::Record *record;
  bool isVariant;
  std::vector<Member> children;
};

bool byMemberLoc(const Member &a, const Member &b) {
  return byLoc(a.record, b.record);
}

/// Build the children of `parent` recursively. Concrete Nodes and
/// sub-Variants are gathered into one vector and sorted by source
/// location, so the emitted layout follows `.td` declaration order
/// regardless of which group an entry belongs to.
std::vector<Member>
buildChildren(const llvm::Record *parent,
              const std::vector<const llvm::Record *> &variants,
              const std::vector<const llvm::Record *> &nodes) {
  std::vector<Member> out;
  for (const llvm::Record *n : nodes) {
    if (n->getValueAsDef("Parent") == parent) {
      out.push_back({n, false, {}});
    }
  }
  for (const llvm::Record *sv : variants) {
    if (sv->getValueAsDef("Parent") == parent) {
      out.push_back({sv, true, buildChildren(sv, variants, nodes)});
    }
  }
  std::sort(out.begin(), out.end(), byMemberLoc);
  return out;
}

/// Top-level variants are those parented at the Base. Returns them in
/// source order, each populated with its descendant tree.
std::vector<Member>
buildRoots(const std::vector<const llvm::Record *> &variants,
           const std::vector<const llvm::Record *> &nodes) {
  std::vector<Member> roots;
  for (const llvm::Record *v : variants) {
    if (!v->getValueAsDef("Parent")->isSubClassOf("Variant")) {
      roots.push_back({v, true, buildChildren(v, variants, nodes)});
    }
  }
  std::sort(roots.begin(), roots.end(), byMemberLoc);
  return roots;
}

/// Collect every transitive concrete leaf of `v`, depth-first in source
/// order. Used by `to<V>Kind` to enumerate the `HirKind` values that can
/// actually appear on a node within this variant's subtree.
void collectLeaves(const Member &v, std::vector<const llvm::Record *> &out) {
  for (const Member &c : v.children) {
    if (c.isVariant) {
      collectLeaves(c, out);
    } else {
      out.push_back(c.record);
    }
  }
}

//===----------------------------------------------------------------------===//
// HirKind enum emission
//===----------------------------------------------------------------------===//

/// Emit one variant block: FIRST sentinel, the variant itself, every
/// child (recursing into sub-variants), then LAST sentinel. Nested layout
/// makes parent-level range checks include every transitive descendant.
void emitVariantBlock(CodeFormatter &fmt, const Member &v) {
  const std::string name = v.record->getName().str();
  const std::string upper = llvm::StringRef(name).upper();
  fmt.linef("// {0}", name);
  fmt.linef("{0}_FIRST,", upper);
  fmt.linef("{0},", name);
  for (const Member &c : v.children) {
    if (c.isVariant) {
      emitVariantBlock(fmt, c);
    } else {
      fmt.linef("{0},", c.record->getName().str());
    }
  }
  fmt.linef("{0}_LAST,", upper);
}

void emitVariants(CodeFormatter &fmt, const std::vector<Member> &roots) {
  for (const Member &v : roots) {
    emitVariantBlock(fmt, v);
    fmt.line("");
  }
}

/// Emit Nodes whose `Parent` is the Base rather than a Variant, bracketed
/// by `NODES_FIRST`/`NODES_LAST` sentinels for symmetry with the Variant
/// blocks. Sentinels are emitted unconditionally so the layout is stable
/// even when no base-parented nodes exist.
void emitBaseNodes(CodeFormatter &fmt,
                   const std::vector<const llvm::Record *> &nodes) {
  std::vector<const llvm::Record *> baseNodes;
  for (const llvm::Record *n : nodes) {
    if (!n->getValueAsDef("Parent")->isSubClassOf("Variant")) {
      baseNodes.push_back(n);
    }
  }
  std::sort(baseNodes.begin(), baseNodes.end(), byLoc);

  fmt.line("// Nodes");
  fmt.line("NODES_FIRST,");
  for (const llvm::Record *n : baseNodes) {
    fmt.linef("{0},", n->getName().str());
  }
  fmt.line("NODES_LAST,");
  fmt.line("");
}

//===----------------------------------------------------------------------===//
// asString(HirKind)
//===----------------------------------------------------------------------===//

void emitAsStringVariant(CodeFormatter &fmt, llvm::StringRef treeName,
                         const Member &v) {
  const std::string name = v.record->getName().str();
  const std::string upper = llvm::StringRef(name).upper();
  fmt.linef("case {0}Kind::{1}_FIRST: return \"{1}_FIRST\";", treeName, upper);
  fmt.linef("case {0}Kind::{1}: return \"{1}\";", treeName, name);
  for (const Member &c : v.children) {
    if (c.isVariant) {
      emitAsStringVariant(fmt, treeName, c);
    } else {
      const std::string n = c.record->getName().str();
      fmt.linef("case {0}Kind::{1}: return \"{1}\";", treeName, n);
    }
  }
  fmt.linef("case {0}Kind::{1}_LAST: return \"{1}_LAST\";", treeName, upper);
}

/// Emit `asString(HirKind)`, mapping every enumerator (including the
/// `*_FIRST` / `*_LAST` sentinels and the System block) to its name.
/// Keeping sentinel cases makes the switch exhaustive without any
/// `default:` arm, silencing `-Wswitch` regardless of how the consumer
/// compiles.
void emitAsString(CodeFormatter &fmt, llvm::StringRef treeName,
                  const std::vector<Member> &roots,
                  const std::vector<const llvm::Record *> &nodes) {
  fmt.linef("inline std::string asString({0}Kind kind) {{", treeName);
  {
    auto body = fmt.block();
    fmt.line("switch (kind) {");

    for (const Member &v : roots) {
      emitAsStringVariant(fmt, treeName, v);
    }

    std::vector<const llvm::Record *> baseNodes;
    for (const llvm::Record *n : nodes) {
      if (!n->getValueAsDef("Parent")->isSubClassOf("Variant")) {
        baseNodes.push_back(n);
      }
    }
    std::sort(baseNodes.begin(), baseNodes.end(), byLoc);
    fmt.linef("case {0}Kind::NODES_FIRST: return \"NODES_FIRST\";", treeName);
    for (const llvm::Record *n : baseNodes) {
      fmt.linef("case {0}Kind::{1}: return \"{1}\";", treeName,
                n->getName().str());
    }
    fmt.linef("case {0}Kind::NODES_LAST: return \"NODES_LAST\";", treeName);

    fmt.linef("case {0}Kind::SYSTEM_FIRST: return \"SYSTEM_FIRST\";", treeName);
    fmt.linef("case {0}Kind::Error: return \"Error\";", treeName);
    fmt.linef("case {0}Kind::Tombstone: return \"Tombstone\";", treeName);
    fmt.linef("case {0}Kind::SYSTEM_LAST: return \"SYSTEM_LAST\";", treeName);
    fmt.line("}");
    fmt.line("");
    fmt.line("util::yuzu_unreachable();");
  }
  fmt.line("}");
}

//===----------------------------------------------------------------------===//
// Per-variant <V>Kind enums
//===----------------------------------------------------------------------===//
//
// For each variant `V`, emit a narrow enum listing only `V`'s direct
// children (concrete Nodes by their own name, sub-Variants by the
// variant's name) plus the conversion from `HirKind`. Callers dispatch
// one level at a time and `-Wswitch` enforces exhaustiveness — adding a
// new direct child of `V` breaks every `switch (x.getXKind())`.

void emitVariantKind(CodeFormatter &fmt, llvm::StringRef treeName,
                     const Member &v) {
  const std::string name = v.record->getName().str();

  fmt.linef("enum class {0}Kind : uint8_t {{", name);
  {
    auto body = fmt.block();
    for (const Member &c : v.children) {
      fmt.linef("{0},", c.record->getName().str());
    }
  }
  fmt.line("};");
  fmt.line("");

  fmt.linef("inline {0}Kind to{0}Kind({1}Kind kind) {{", name, treeName);
  {
    auto body = fmt.block();
    fmt.line("switch (kind) {");
    for (const Member &c : v.children) {
      const std::string childName = c.record->getName().str();
      if (c.isVariant) {
        std::vector<const llvm::Record *> leaves;
        collectLeaves(c, leaves);
        for (std::size_t i = 0; i < leaves.size(); ++i) {
          if (i + 1 < leaves.size()) {
            fmt.linef("case {0}Kind::{1}:", treeName,
                      leaves[i]->getName().str());
          } else {
            fmt.linef("case {0}Kind::{1}: return {2}Kind::{3};", treeName,
                      leaves[i]->getName().str(), name, childName);
          }
        }
      } else {
        fmt.linef("case {0}Kind::{1}: return {2}Kind::{1};", treeName,
                  childName, name);
      }
    }
    fmt.line("default: util::yuzu_unreachable();");
    fmt.line("}");
  }
  fmt.line("}");
  fmt.line("");

  fmt.linef("inline std::string asString({0}Kind kind) {{", name);
  {
    auto body = fmt.block();
    fmt.line("switch (kind) {");
    for (const Member &c : v.children) {
      const std::string childName = c.record->getName().str();
      fmt.linef("case {0}Kind::{1}: return \"{1}\";", name, childName);
    }
    fmt.line("}");
    fmt.line("");
    fmt.line("util::yuzu_unreachable();");
  }
  fmt.line("}");
  fmt.line("");

  for (const Member &c : v.children) {
    if (c.isVariant) {
      emitVariantKind(fmt, treeName, c);
    }
  }
}

void emitVariantKinds(CodeFormatter &fmt, llvm::StringRef treeName,
                      const std::vector<Member> &roots) {
  for (const Member &v : roots) {
    emitVariantKind(fmt, treeName, v);
  }
}

} // namespace

void HirKindGenerator::generate(const llvm::RecordKeeper &records) {
  const std::string ns = findNamespace(records, "Base");
  const llvm::StringRef treeName = findTreeName(records, "Base");

  const std::vector<const llvm::Record *> variants =
      records.getAllDerivedDefinitions("Variant");
  const std::vector<const llvm::Record *> nodes =
      records.getAllDerivedDefinitions("Node");

  const std::vector<Member> roots = buildRoots(variants, nodes);

  // variants + nodes + 2 sentinels per variant
  //   + 2 sentinels (NODES_FIRST/LAST)
  //   + 2 sentinels (SYSTEM_FIRST/LAST) + 2 system kinds (Error, Tombstone)
  const std::size_t total =
      variants.size() + nodes.size() + 2 * variants.size() + 2 + 2 + 2;

  fmt.linef("namespace {0} {{", ns);
  fmt.line("");
  fmt.linef("enum class {0}Kind : {1} {{", treeName, getUnderlyingType(total));
  {
    auto body = fmt.block();
    emitVariants(fmt, roots);
    emitBaseNodes(fmt, nodes);

    fmt.line("// System");
    fmt.line("SYSTEM_FIRST,");
    fmt.line("Error,");
    fmt.line("Tombstone,");
    fmt.line("SYSTEM_LAST,");
  }
  fmt.line("};");
  fmt.line("");
  emitAsString(fmt, treeName, roots, nodes);
  fmt.line("");
  emitVariantKinds(fmt, treeName, roots);
  fmt.linef("} // namespace {0}", ns);
}

} // namespace yuzu::tools
