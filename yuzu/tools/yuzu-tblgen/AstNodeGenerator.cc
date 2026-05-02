#include "AstNodeGenerator.h"

#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TableGen/Main.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/TableGenBackend.h"

#include <algorithm>
#include <cstdint>
#include <string>

namespace yuzu_tools {

namespace node {
void emitCanCast(llvm::raw_ostream &os, const llvm::Record *record) noexcept {
  const llvm::StringRef name = record->getValueAsString("Name");
  os << llvm::formatv(
      R"(
      [[nodiscard]] static bool canCast(SyntaxKind kind) noexcept {
        return kind == SyntaxKind::{0};
      }
      )",
      name);
}

void emitCast(llvm::raw_ostream &os, const llvm::Record *record) noexcept {
  const llvm::StringRef name = record->getValueAsString("Name");
  os << llvm::formatv(
      R"(
      [[nodiscard]] static std::optional<{0}>
      cast(syntax::SyntaxNode node) noexcept {
        const SyntaxKind kind = static_cast<SyntaxKind>(node.getKind());
        
        if ({0}::canCast(kind)) {
          return std::make_optional<{0}>(std::move(node));
        }

        return std::nullopt;
      }
      )",
      name);
}

void emitChildAstMethod(llvm::raw_ostream &os,
                        const llvm::Record *method) noexcept {
  const llvm::StringRef name = method->getValueAsString("Name");
  const int64_t n = method->getValueAsInt("N");
  const llvm::StringRef subclass = method->getValueAsString("AstNodeSubclass");

  os << llvm::formatv(
      R"(
        [[nodiscard]] std::unique_ptr<{2}>
        get{0}() const noexcept {
          return child<{2}>(node, {1});
        }
        )",
      name, n, subclass);
}

void emitTokenAstMethod(llvm::raw_ostream &os,
                        const llvm::Record *method) noexcept {
  const llvm::StringRef name = method->getValueAsString("Name");
  const llvm::StringRef kind = method->getValueAsString("Kind");
  const int64_t n = method->getValueAsInt("N");

  // When kind is empty, then defer the implementation to a source file. Most
  // nodes will be represented by a single SyntaxKind.
  if (kind == "") {
    os << llvm::formatv(
        R"(
        [[nodiscard]] std::optional<syntax::SyntaxToken>
        get{0}() const noexcept;
        )",
        name, kind, n);
  }
  // When kind is not empty, inline the implementation.
  else {
    os << llvm::formatv(
        R"(
        [[nodiscard]] std::optional<syntax::SyntaxToken>
        get{0}() const noexcept {
          return token(node, SyntaxKind::{1}, {2});
        }
        )",
        name, kind, n);
  }
}

void emitAstMethods(llvm::raw_ostream &os,
                    const llvm::Record *record) noexcept {
  for (const auto &method : record->getValueAsListOfDefs("Methods")) {
    if (method->isSubClassOf("ChildAstMethod")) {
      emitChildAstMethod(os, method);
    } else if (method->isSubClassOf("TokenAstMethod")) {
      emitTokenAstMethod(os, method);
    }
  }
}
} // namespace node

namespace variant {
void emitCanCast(
    llvm::raw_ostream &os, const llvm::Record *record,
    const llvm::ArrayRef<const llvm::Record *> derivedDefinitions) noexcept {
  const auto ifStatements = llvm::map_range(
      derivedDefinitions, [](const llvm::Record *record) -> std::string {
        const llvm::StringRef Name = record->getValueAsString("Name");
        return llvm::formatv(
            R"(if (std::holds_alternative<{0}>(value)) {{
                  return true;
                }
            )",
            Name);
      });

  const llvm::StringRef name = record->getValueAsString("Name");
  os << llvm::formatv(
      R"(
      [[nodiscard]] static bool canCast({0} value) noexcept {{
        {1}

        return false;
      }
      )",
      name, llvm::join(ifStatements, "\n"));
}

void emitCast(
    llvm::raw_ostream &os, const llvm::Record *record,
    const llvm::ArrayRef<const llvm::Record *> derivedDefinitions) noexcept {
  const llvm::StringRef name = record->getValueAsString("Name");

  const auto ifStatements =
      llvm::map_range(derivedDefinitions,
                      [name](const llvm::Record *derivedRecord) -> std::string {
                        const llvm::StringRef derivedName =
                            derivedRecord->getValueAsString("Name");
                        return llvm::formatv(
                            R"(
                            if ({1}::canCast(kind)) {{
                              return std::make_optional<{0}>({1}(std::move(node)));
                            }
                            )",
                            name, derivedName);
                      });

  os << llvm::formatv(
      R"(
      [[nodiscard]] static std::optional<{0}>
      cast(syntax::SyntaxNode node) noexcept {
        const SyntaxKind kind = static_cast<SyntaxKind>(node.getKind());
        
        {1}
        
        return std::nullopt;
      }
      )",
      name, llvm::join(ifStatements, "\n"));
}
} // namespace variant

void AstNodeGenerator::emitClassDefinitions(
    llvm::raw_ostream &os) const noexcept {
  const llvm::Record *grammar = records.getDef("YuzuGrammar");

  const std::vector<const llvm::Record *> nodes =
      grammar->getValueAsListOfDefs("Nodes");

  // Generate abstract node forward declarations.
  std::vector<const llvm::Record *> abstractNodes;
  std::copy_if(nodes.begin(), nodes.end(), std::back_inserter(abstractNodes),
               [](const llvm::Record *record) {
                 return record->isSubClassOf("AbstractAstNode");
               });

  for (const llvm::Record *abstractNode : abstractNodes) {
    os << "class " << abstractNode->getValueAsString("Name") << ";\n";
  }

  // Generate concrete node defs.
  std::vector<const llvm::Record *> concreteNodes;
  std::copy_if(nodes.begin(), nodes.end(), std::back_inserter(concreteNodes),
               [](const llvm::Record *record) {
                 return record->isSubClassOf("ConcreteAstNode");
               });

  for (const llvm::Record *concreteNode : concreteNodes) {
    os << llvm::formatv(
        R"(
        class {0} final : public AstNode<{0}> {{
        public:
          explicit {0}(syntax::SyntaxNode node) : AstNode(std::move(node)) {{}

          {0}() = delete;

        )",
        concreteNode->getValueAsString("Name"));

    node::emitCanCast(os, concreteNode);
    node::emitCast(os, concreteNode);
    node::emitAstMethods(os, concreteNode);

    os << "\n};\n\n";
  }

  // Generate abstract node defs.
  for (const llvm::Record *abstractNode : abstractNodes) {
    // Collect the derived defs.
    std::vector<const llvm::Record *> derivedDefs;
    std::copy_if(nodes.begin(), nodes.end(), std::back_inserter(derivedDefs),
                 [abstractNode](const llvm::Record *record) {
                   return record->getValueAsString("Parent") ==
                          abstractNode->getValueAsString("Name");
                 });

    const std::string VariantClasses =
        llvm::join(llvm::map_range(derivedDefs,
                                   [](const auto &Subclass) {
                                     return Subclass->getValueAsString("Name");
                                   }),
                   ", ");

    os << llvm::formatv(
        R"(
        class {0} : public std::variant<{1}> {{
        public:
          using std::variant<{1}>::variant;

          {0}() = delete;

        )",
        abstractNode->getValueAsString("Name"), VariantClasses);

    variant::emitCanCast(os, abstractNode, derivedDefs);
    variant::emitCast(os, abstractNode, derivedDefs);

    os << "\n};\n\n";
  }
}

void AstNodeGenerator::emitHeader(llvm::raw_ostream &os) const noexcept {
  emitSourceFileHeader("Yuzu AST Node Declarations", os);

  emitOpenIncludeGuards(os);
  emitIncludes(os);

  os << "namespace yuzu::ast {\n";

  emitClassDefinitions(os);

  os << "} // namespace yuzu::ast\n\n";

  emitCloseIncludeGuards(os);
}

void AstNodeGenerator::runImpl(llvm::raw_ostream &os) const noexcept {
  emitHeader(os);
}
} // namespace yuzu_tools
