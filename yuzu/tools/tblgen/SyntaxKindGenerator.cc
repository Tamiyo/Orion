#include "SyntaxKindGenerator.h"

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
  if (count <= std::numeric_limits<std::uint8_t>::max())
    return "uint8_t";
  if (count <= std::numeric_limits<std::uint16_t>::max())
    return "uint16_t";
  if (count <= std::numeric_limits<std::uint32_t>::max())
    return "uint32_t";
  return "uint64_t";
}

/// Sort defs by source position so the emitted enum follows include/file
/// order. `getAllDerivedDefinitions` returns alphabetical (RecordKeeper backs
/// defs with a `std::map<std::string, ...>`), so we re-sort here.
bool byLoc(const llvm::Record *a, const llvm::Record *b) {
  return a->getLoc().front().getPointer() < b->getLoc().front().getPointer();
}

void emitTokens(CodeFormatter &fmt,
                const std::vector<const llvm::Record *> &tokens) {
  fmt.line("// Tokens");
  fmt.line("TOKENS_FIRST,");
  for (const llvm::Record *t : tokens) {
    fmt.linef("{0},", t->getName().str());
  }
  fmt.line("TOKENS_LAST,");
  fmt.line("");
}

/// True if `n`'s `Parent` is a Variant. False when the Node is parented
/// directly at the Base — those don't get range sentinels because they
/// don't form a discriminable family.
bool isUnderVariant(const llvm::Record *n) {
  return n->getValueAsDef("Parent")->isSubClassOf("Variant");
}

void emitNodes(CodeFormatter &fmt,
               const std::vector<const llvm::Record *> &variants,
               const std::vector<const llvm::Record *> &nodes) {
  for (const llvm::Record *v : variants) {
    // Concrete Node children: those whose `parent` field points at this
    // variant. Sort by source position so the section follows file order.
    std::vector<const llvm::Record *> children;
    for (const llvm::Record *n : nodes) {
      if (n->getValueAsDef("Parent") == v) {
        children.push_back(n);
      }
    }
    std::sort(children.begin(), children.end(), byLoc);

    const std::string upper = llvm::StringRef(v->getName()).upper();
    fmt.linef("// {0}", v->getName().str());
    fmt.linef("{0}_FIRST,", upper);
    fmt.linef("{0},", v->getName().str());
    for (const llvm::Record *n : children) {
      fmt.linef("{0},", n->getName().str());
    }
    fmt.linef("{0}_LAST,", upper);
    fmt.line("");
  }
}

/// Emit Nodes whose `Parent` is the Base rather than a Variant, bracketed
/// by `NODES_FIRST`/`NODES_LAST` sentinels for symmetry with the
/// `<VARIANT>_FIRST`/`_LAST` blocks. The sentinels are emitted unconditionally
/// so consumers can rely on a stable layout — an empty group still yields
/// adjacent `NODES_FIRST, NODES_LAST,`.
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

/// Emit `asString(SyntaxKind)`, mapping every enumerator (including the
/// `*_FIRST` / `*_LAST` sentinels and the System block) to its name. Keeping
/// sentinel cases makes the switch exhaustive without any `default:` arm,
/// silencing `-Wswitch` regardless of how the consumer compiles.
void emitAsString(CodeFormatter &fmt,
                  const std::vector<const llvm::Record *> &tokens,
                  const std::vector<const llvm::Record *> &variants,
                  const std::vector<const llvm::Record *> &nodes) {
  fmt.line("inline std::string asString(SyntaxKind kind) {");
  {
    auto body = fmt.block();
    fmt.line("switch (kind) {");

    fmt.line("case SyntaxKind::TOKENS_FIRST: return \"TOKENS_FIRST\";");
    for (const llvm::Record *t : tokens) {
      fmt.linef("case SyntaxKind::{0}: return \"{0}\";", t->getName().str());
    }
    fmt.line("case SyntaxKind::TOKENS_LAST: return \"TOKENS_LAST\";");

    for (const llvm::Record *v : variants) {
      std::vector<const llvm::Record *> children;
      for (const llvm::Record *n : nodes) {
        if (n->getValueAsDef("Parent") == v) {
          children.push_back(n);
        }
      }
      std::sort(children.begin(), children.end(), byLoc);

      const std::string upper = llvm::StringRef(v->getName()).upper();
      fmt.linef("case SyntaxKind::{0}_FIRST: return \"{0}_FIRST\";", upper);
      fmt.linef("case SyntaxKind::{0}: return \"{0}\";", v->getName().str());
      for (const llvm::Record *n : children) {
        fmt.linef("case SyntaxKind::{0}: return \"{0}\";", n->getName().str());
      }
      fmt.linef("case SyntaxKind::{0}_LAST: return \"{0}_LAST\";", upper);
    }

    std::vector<const llvm::Record *> baseNodes;
    for (const llvm::Record *n : nodes) {
      if (!isUnderVariant(n)) {
        baseNodes.push_back(n);
      }
    }
    std::sort(baseNodes.begin(), baseNodes.end(), byLoc);
    fmt.line("case SyntaxKind::NODES_FIRST: return \"NODES_FIRST\";");
    for (const llvm::Record *n : baseNodes) {
      fmt.linef("case SyntaxKind::{0}: return \"{0}\";", n->getName().str());
    }
    fmt.line("case SyntaxKind::NODES_LAST: return \"NODES_LAST\";");

    fmt.line("case SyntaxKind::SYSTEM_FIRST: return \"SYSTEM_FIRST\";");
    fmt.line("case SyntaxKind::Error: return \"Error\";");
    fmt.line("case SyntaxKind::Tombstone: return \"Tombstone\";");
    fmt.line("case SyntaxKind::SYSTEM_LAST: return \"SYSTEM_LAST\";");
    fmt.line("}");
    fmt.line("");
    fmt.line("util::yuzu_unreachable();");
  }
  fmt.line("}");
}

} // namespace

void SyntaxKindGenerator::generate(const llvm::RecordKeeper &records) {
  const std::string ns = findNamespace(records, "Base");

  std::vector<const llvm::Record *> tokens =
      records.getAllDerivedDefinitions("Metadata");
  std::vector<const llvm::Record *> variants =
      records.getAllDerivedDefinitions("Variant");
  const std::vector<const llvm::Record *> nodes =
      records.getAllDerivedDefinitions("Node");

  std::sort(tokens.begin(), tokens.end(), byLoc);
  std::sort(variants.begin(), variants.end(), byLoc);

  // tokens + 2 sentinels (TOKENS_FIRST/LAST)
  //   + variants + nodes + 2 sentinels per variant
  //   + 2 sentinels (NODES_FIRST/LAST)
  //   + 2 sentinels (SYSTEM_FIRST/LAST) + 2 system kinds (Error, Tombstone)
  const std::size_t total = tokens.size() + 2 + variants.size() + nodes.size() +
                            2 * variants.size() + 2 + 2 + 2;

  fmt.linef("namespace {0} {{", ns);
  fmt.line("");
  fmt.linef("enum class SyntaxKind : {0} {{", getUnderlyingType(total));
  {
    auto body = fmt.block();
    emitTokens(fmt, tokens);
    emitNodes(fmt, variants, nodes);
    emitBaseNodes(fmt, nodes);

    fmt.line("// System");
    fmt.line("SYSTEM_FIRST,");
    fmt.line("Error,");
    fmt.line("Tombstone,");
    fmt.line("SYSTEM_LAST,");
  }
  fmt.line("};");
  fmt.line("");
  emitAsString(fmt, tokens, variants, nodes);
  fmt.linef("} // namespace {0}", ns);
}

} // namespace yuzu::tools
