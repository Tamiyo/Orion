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
  //   + 2 system kinds (Error, Tombstone)
  const std::size_t total = tokens.size() + 2 +
                            variants.size() + nodes.size() +
                            2 * variants.size() + 2;

  fmt.linef("namespace {0} {{", ns);
  fmt.line("");
  fmt.linef("enum class SyntaxKind : {0} {{", getUnderlyingType(total));
  {
    auto body = fmt.block();
    emitTokens(fmt, tokens);
    emitNodes(fmt, variants, nodes);

    fmt.line("// System");
    fmt.line("Error,");
    fmt.line("Tombstone,");
  }
  fmt.line("};");
  fmt.linef("} // namespace {0}", ns);
}

} // namespace yuzu::tools
