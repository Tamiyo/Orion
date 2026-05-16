#include "SyntaxKindGenerator.h"

#include "utils/SchemaUtils.h"
#include "utils/TokenUtils.h"

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

/// True if `v`'s `Parent` is itself a Variant (i.e. `v` is a sub-variant
/// like `Literal : Variant<Expr>`). Top-level variants are parented at
/// the Base.
bool isNestedVariant(const llvm::Record *v) {
  return v->getValueAsDef("Parent")->isSubClassOf("Variant");
}

/// Recursively emit one variant block: `<V>_FIRST`, the variant itself,
/// concrete Node children, every sub-variant nested inside, then
/// `<V>_LAST`. Nested layout makes the parent's range-check `isA`
/// include every transitive descendant — `Expr::isA(IntLit)` is true
/// even when `IntLit` lives under `Literal : Variant<Expr>`.
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

void emitNodes(CodeFormatter &fmt,
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

/// Recursive counterpart to `emitVariantBlock` for `asString`: emit the
/// FIRST sentinel case, the variant case, concrete child cases, every
/// sub-variant nested inside, then the LAST sentinel case. Layout
/// matches the enum exactly so the switch covers every enumerator.
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
  fmt.linef("case SyntaxKind::{0}_FIRST: return \"{0}_FIRST\";", upper);
  fmt.linef("case SyntaxKind::{0}: return \"{0}\";", v->getName().str());
  for (const llvm::Record *n : concreteChildren) {
    fmt.linef("case SyntaxKind::{0}: return \"{0}\";", n->getName().str());
  }
  for (const llvm::Record *sv : subVariants) {
    emitAsStringVariant(fmt, sv, variants, nodes);
  }
  fmt.linef("case SyntaxKind::{0}_LAST: return \"{0}_LAST\";", upper);
}

/// Emit the same per-category predicates `TokenKindGenerator` emits on
/// `TokenKind` (`isSymbol`, `isPunctuation`, `isKeyword`, `isLiteral`,
/// `isTrivia`), so AST consumers can check token categories without
/// casting through `lexer::TokenKind`. Non-token `SyntaxKind` values
/// (variants, nodes, sentinels, system kinds) fall through to `false`.
void emitTokenPredicates(CodeFormatter &fmt,
                         const std::vector<const llvm::Record *> &tokens) {
  std::vector<TokenInfo> infos;
  infos.reserve(tokens.size());
  for (const llvm::Record *r : tokens) {
    infos.push_back(parseTokenInfo(r));
  }

  auto emit = [&](llvm::StringRef name, auto select) {
    fmt.linef("inline bool is{0}(SyntaxKind kind) {{", name);
    {
      auto body = fmt.block();
      fmt.line("switch (kind) {");
      for (const TokenInfo &t : infos) {
        if (select(t)) {
          fmt.linef("case SyntaxKind::{0}:", t.name);
        }
      }
      fmt.line("  return true;");
      fmt.line("default:");
      fmt.line("  return false;");
      fmt.line("}");
    }
    fmt.line("}");
    fmt.line("");
  };

  emit("Symbol", [](const TokenInfo &t) { return t.isSymbol; });
  emit("Punctuation", [](const TokenInfo &t) { return t.isPunctuation; });
  emit("Keyword", [](const TokenInfo &t) { return t.isKeyword; });
  emit("Literal", [](const TokenInfo &t) { return t.isLiteral; });
  emit("Trivia", [](const TokenInfo &t) { return t.isTrivia; });
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
  emitTokenPredicates(fmt, tokens);
  emitAsString(fmt, tokens, variants, nodes);
  fmt.linef("} // namespace {0}", ns);
}

} // namespace yuzu::tools
