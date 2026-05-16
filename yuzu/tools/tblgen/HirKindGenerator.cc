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

/// True if `n`'s `Parent` is a `Variant`. False when the Node is
/// parented directly at the Base — those land in the `// Nodes` block
/// without surrounding sentinels (a single concrete kind has nothing to
/// discriminate against).
bool isUnderVariant(const llvm::Record *n) {
  return n->getValueAsDef("Parent")->isSubClassOf("Variant");
}

/// True if `v`'s `Parent` is itself a Variant. Top-level variants are
/// parented at the Base.
bool isNestedVariant(const llvm::Record *v) {
  return v->getValueAsDef("Parent")->isSubClassOf("Variant");
}

/// Recursively emit one variant block: FIRST sentinel, the variant
/// itself, concrete Node children, every sub-variant nested inside,
/// then LAST sentinel. Nested layout makes parent-level range checks
/// include every transitive descendant.
void emitVariantBlock(CodeFormatter &fmt, const llvm::Record *v,
                      const std::vector<const llvm::Record *> &variants,
                      const std::vector<const llvm::Record *> &nodes) {
  std::vector<const llvm::Record *> concreteChildren;
  for (const llvm::Record *n : nodes) {
    if (n->getValueAsDef("Parent") == v) {
      concreteChildren.push_back(n);
    }
  }
  std::sort(concreteChildren.begin(), concreteChildren.end(), byLoc);

  std::vector<const llvm::Record *> subVariants;
  for (const llvm::Record *sv : variants) {
    if (sv->getValueAsDef("Parent") == v) {
      subVariants.push_back(sv);
    }
  }
  std::sort(subVariants.begin(), subVariants.end(), byLoc);

  const std::string upper = llvm::StringRef(v->getName()).upper();
  fmt.linef("// {0}", v->getName().str());
  fmt.linef("{0}_FIRST,", upper);
  fmt.linef("{0},", v->getName().str());
  for (const llvm::Record *n : concreteChildren) {
    fmt.linef("{0},", n->getName().str());
  }
  for (const llvm::Record *sv : subVariants) {
    emitVariantBlock(fmt, sv, variants, nodes);
  }
  fmt.linef("{0}_LAST,", upper);
}

void emitVariants(CodeFormatter &fmt,
                  const std::vector<const llvm::Record *> &variants,
                  const std::vector<const llvm::Record *> &nodes) {
  for (const llvm::Record *v : variants) {
    if (isNestedVariant(v)) {
      continue; // emitted recursively by its parent's block
    }
    emitVariantBlock(fmt, v, variants, nodes);
    fmt.line("");
  }
}

/// Emit Nodes whose `Parent` is the Base rather than a Variant,
/// bracketed by `NODES_FIRST`/`NODES_LAST` sentinels for symmetry with
/// the Variant blocks. Sentinels are emitted unconditionally so the
/// layout is stable even when no base-parented nodes exist.
void emitBaseNodes(CodeFormatter &fmt,
                   const std::vector<const llvm::Record *> &nodes) {
  std::vector<const llvm::Record *> baseNodes;
  for (const llvm::Record *n : nodes) {
    if (!isUnderVariant(n)) {
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

/// Recursive counterpart to `emitVariantBlock` for `asString`: emit
/// FIRST/variant/children/sub-variants/LAST cases so the switch covers
/// every enumerator the enum produced.
void emitAsStringVariant(CodeFormatter &fmt, const llvm::Record *v,
                         const std::vector<const llvm::Record *> &variants,
                         const std::vector<const llvm::Record *> &nodes) {
  std::vector<const llvm::Record *> concreteChildren;
  for (const llvm::Record *n : nodes) {
    if (n->getValueAsDef("Parent") == v) {
      concreteChildren.push_back(n);
    }
  }
  std::sort(concreteChildren.begin(), concreteChildren.end(), byLoc);

  std::vector<const llvm::Record *> subVariants;
  for (const llvm::Record *sv : variants) {
    if (sv->getValueAsDef("Parent") == v) {
      subVariants.push_back(sv);
    }
  }
  std::sort(subVariants.begin(), subVariants.end(), byLoc);

  const std::string upper = llvm::StringRef(v->getName()).upper();
  fmt.linef("case HirKind::{0}_FIRST: return \"{0}_FIRST\";", upper);
  fmt.linef("case HirKind::{0}: return \"{0}\";", v->getName().str());
  for (const llvm::Record *n : concreteChildren) {
    fmt.linef("case HirKind::{0}: return \"{0}\";", n->getName().str());
  }
  for (const llvm::Record *sv : subVariants) {
    emitAsStringVariant(fmt, sv, variants, nodes);
  }
  fmt.linef("case HirKind::{0}_LAST: return \"{0}_LAST\";", upper);
}

/// Emit `asString(HirKind)`, mapping every enumerator (including the
/// `*_FIRST` / `*_LAST` sentinels and the System block) to its name.
/// Keeping sentinel cases makes the switch exhaustive without any
/// `default:` arm, silencing `-Wswitch` regardless of how the consumer
/// compiles.
void emitAsString(CodeFormatter &fmt,
                  const std::vector<const llvm::Record *> &variants,
                  const std::vector<const llvm::Record *> &nodes) {
  fmt.line("inline std::string asString(HirKind kind) {");
  {
    auto body = fmt.block();
    fmt.line("switch (kind) {");

    for (const llvm::Record *v : variants) {
      if (isNestedVariant(v)) {
        continue; // emitted recursively by its parent's block
      }
      emitAsStringVariant(fmt, v, variants, nodes);
    }

    std::vector<const llvm::Record *> baseNodes;
    for (const llvm::Record *n : nodes) {
      if (!isUnderVariant(n)) {
        baseNodes.push_back(n);
      }
    }
    std::sort(baseNodes.begin(), baseNodes.end(), byLoc);
    fmt.line("case HirKind::NODES_FIRST: return \"NODES_FIRST\";");
    for (const llvm::Record *n : baseNodes) {
      fmt.linef("case HirKind::{0}: return \"{0}\";", n->getName().str());
    }
    fmt.line("case HirKind::NODES_LAST: return \"NODES_LAST\";");

    fmt.line("case HirKind::SYSTEM_FIRST: return \"SYSTEM_FIRST\";");
    fmt.line("case HirKind::Error: return \"Error\";");
    fmt.line("case HirKind::Tombstone: return \"Tombstone\";");
    fmt.line("case HirKind::SYSTEM_LAST: return \"SYSTEM_LAST\";");
    fmt.line("}");
    fmt.line("");
    fmt.line("util::yuzu_unreachable();");
  }
  fmt.line("}");
}

} // namespace

void HirKindGenerator::generate(const llvm::RecordKeeper &records) {
  const std::string ns = findNamespace(records, "Base");

  std::vector<const llvm::Record *> variants =
      records.getAllDerivedDefinitions("Variant");
  const std::vector<const llvm::Record *> nodes =
      records.getAllDerivedDefinitions("Node");

  std::sort(variants.begin(), variants.end(), byLoc);

  // variants + nodes + 2 sentinels per variant
  //   + 2 sentinels (NODES_FIRST/LAST)
  //   + 2 sentinels (SYSTEM_FIRST/LAST) + 2 system kinds (Error, Tombstone)
  const std::size_t total =
      variants.size() + nodes.size() + 2 * variants.size() + 2 + 2 + 2;

  fmt.linef("namespace {0} {{", ns);
  fmt.line("");
  fmt.linef("enum class HirKind : {0} {{", getUnderlyingType(total));
  {
    auto body = fmt.block();
    emitVariants(fmt, variants, nodes);
    emitBaseNodes(fmt, nodes);

    fmt.line("// System");
    fmt.line("SYSTEM_FIRST,");
    fmt.line("Error,");
    fmt.line("Tombstone,");
    fmt.line("SYSTEM_LAST,");
  }
  fmt.line("};");
  fmt.line("");
  emitAsString(fmt, variants, nodes);
  fmt.linef("} // namespace {0}", ns);
}

} // namespace yuzu::tools
