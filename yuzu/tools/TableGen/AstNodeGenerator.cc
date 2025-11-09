#include "yuzu/tools/TableGen/AstNodeGenerator.h"

#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TableGen/Main.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/TableGenBackend.h"

#include <cstdint>
#include <string>
#include <utility>

namespace yuzu_tools {

namespace node {
void emitCanCast(llvm::raw_ostream &os, const llvm::Record *record) noexcept {
  const llvm::StringRef syntaxKind = record->getValueAsString("SyntaxKind");
  os << llvm::formatv(
      R"(
      [[nodiscard]] static bool canCast(SyntaxKind kind) noexcept {
        return kind == SyntaxKind::{0};
      }
      )",
      syntaxKind);
}

void emitCast(llvm::raw_ostream &os, const llvm::Record *record) noexcept {
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
      record->getName());
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
        llvm::StringRef Name = record->getName();
        return llvm::formatv(
            R"(if (std::holds_alternative<{0}>(value)) {{
                  return true;
                }
            )",
            Name);
      });

  os << llvm::formatv(
      R"(
      [[nodiscard]] static bool canCast({0} value) noexcept {{
        {1}

        return false;
      }
      )",
      record->getName(), llvm::join(ifStatements, "\n"));
}

void emitCast(
    llvm::raw_ostream &os, const llvm::Record *record,
    const llvm::ArrayRef<const llvm::Record *> derivedDefinitions) noexcept {
  const auto ifStatements = llvm::map_range(
      derivedDefinitions,
      [record](const llvm::Record *DerivedRecord) -> std::string {
        llvm::StringRef Derivedname = DerivedRecord->getName();
        return llvm::formatv(
            R"(
              if ({1}::canCast(kind)) {{
                return std::make_optional<{0}>({1}(std::move(node)));
              }
              )",
            record->getName(), Derivedname);
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
      record->getName(), llvm::join(ifStatements, "\n"));
}
} // namespace variant

std::string AstNodeGenerator::getIncludeGuardName() const noexcept {
  const llvm::StringRef Basename =
      llvm::sys::path::stem(records.getInputFilename());
  return "YUZU_AST_" + Basename.upper() + "_INC_H";
}

void AstNodeGenerator::emitClassDefinitions(
    llvm::raw_ostream &os) const noexcept {
  (void)os;

  for (const auto &[Name, record] : records.getClasses()) {
    if (isIgnored(record.get())) {
      continue;
    }

    os << "class " << Name << ";\n";
  }

  for (const auto &[Name, record] : records.getDefs()) {
    if (isIgnored(record.get())) {
      continue;
    }

    os << llvm::formatv(
        R"(
        class {0} final : public AstNode<{0}> {{
        public:
          explicit {0}(syntax::SyntaxNode node) : AstNode(std::move(node)) {{}

          {0}() = delete;

        )",
        Name);

    node::emitCanCast(os, record.get());
    node::emitCast(os, record.get());

    node::emitAstMethods(os, record.get());

    os << "\n};\n\n";
  }

  for (const auto &[name, record] : records.getClasses()) {
    if (isIgnored(record.get())) {
      continue;
    }

    const auto derivedDefinitions =
        records.getAllDerivedDefinitionsIfDefined(name);

    const std::string VariantClasses =
        llvm::join(llvm::map_range(derivedDefinitions,
                                   [](const auto &Subclass) {
                                     return Subclass->getName();
                                   }),
                   ", ");

    os << llvm::formatv(
        R"(
        class {0} : public std::variant<{1}> {{
        public:
          using std::variant<{1}>::variant;

          {0}() = delete;

        )",
        name, VariantClasses);

    variant::emitCanCast(os, record.get(), derivedDefinitions);
    variant::emitCast(os, record.get(), derivedDefinitions);

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
