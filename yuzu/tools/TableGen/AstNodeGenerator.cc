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
void emitCanCast(llvm::raw_ostream &OS, const llvm::Record *Record) noexcept {
  const llvm::StringRef SyntaxKind = Record->getValueAsString("SyntaxKind");
  OS << llvm::formatv(
      R"(
      [[nodiscard]] static bool canCast(SyntaxKind Kind) noexcept {
        return Kind == SyntaxKind::{0};
      }
      )",
      SyntaxKind);
}

void emitCast(llvm::raw_ostream &OS, const llvm::Record *Record) noexcept {
  OS << llvm::formatv(
      R"(
      [[nodiscard]] static std::optional<{0}>
      cast(syntax::SyntaxNode Node) noexcept {
        const SyntaxKind Kind = static_cast<SyntaxKind>(Node.getKind());
        
        if ({0}::canCast(Kind)) {
          return std::make_optional<{0}>(std::move(Node));
        }

        return std::nullopt;
      }
      )",
      Record->getName());
}

void emitChildAstMethod(llvm::raw_ostream &OS,
                        const llvm::Record *Method) noexcept {
  const llvm::StringRef Name = Method->getValueAsString("Name");
  const int64_t N = Method->getValueAsInt("N");
  const llvm::StringRef Subclass = Method->getValueAsString("AstNodeSubclass");

  OS << llvm::formatv(
      R"(
        [[nodiscard]] std::unique_ptr<{2}>
        get{0}() const noexcept {
          return child<{2}>(Node_, {1});
        }
        )",
      Name, N, Subclass);
}

void emitTokenAstMethod(llvm::raw_ostream &OS,
                        const llvm::Record *Method) noexcept {
  const llvm::StringRef Name = Method->getValueAsString("Name");
  const llvm::StringRef Kind = Method->getValueAsString("Kind");
  const int64_t N = Method->getValueAsInt("N");

  // When kind is empty, then defer the implementation to a source file. Most
  // nodes will be represented by a single SyntaxKind.
  if (Kind == "") {
    OS << llvm::formatv(
        R"(
        [[nodiscard]] std::optional<syntax::SyntaxToken>
        get{0}() const noexcept;
        )",
        Name, Kind, N);
  }
  // When kind is not empty, inline the implementation.
  else {
    OS << llvm::formatv(
        R"(
        [[nodiscard]] std::optional<syntax::SyntaxToken>
        get{0}() const noexcept {
          return token<(Node_, SyntaxKind::{1}, {2});
        }
        )",
        Name, Kind, N);
  }
}

void emitAstMethods(llvm::raw_ostream &OS,
                    const llvm::Record *Record) noexcept {
  for (const auto &Method : Record->getValueAsListOfDefs("Methods")) {
    if (Method->isSubClassOf("ChildAstMethod")) {
      emitChildAstMethod(OS, Method);
    } else if (Method->isSubClassOf("TokenAstMethod")) {
      emitTokenAstMethod(OS, Method);
    }
  }
}
} // namespace node

namespace variant {
void emitCanCast(
    llvm::raw_ostream &OS, const llvm::Record *Record,
    const llvm::ArrayRef<const llvm::Record *> DerivedDefinitions) noexcept {
  const auto IfStatements = llvm::map_range(
      DerivedDefinitions, [](const llvm::Record *Record) -> std::string {
        llvm::StringRef Name = Record->getName();
        return llvm::formatv(
            R"(if (std::holds_alternative<{0}>(Value)) {{
                  return true;
                }
            )",
            Name);
      });

  OS << llvm::formatv(
      R"(
      [[nodiscard]] static bool canCast({0} Value) noexcept {{
        {1}

        return false;
      }
      )",
      Record->getName(), llvm::join(IfStatements, "\n"));
}

void emitCast(
    llvm::raw_ostream &OS, const llvm::Record *Record,
    const llvm::ArrayRef<const llvm::Record *> DerivedDefinitions) noexcept {
  const auto IfStatements = llvm::map_range(
      DerivedDefinitions,
      [Record](const llvm::Record *DerivedRecord) -> std::string {
        llvm::StringRef Derivedname = DerivedRecord->getName();
        return llvm::formatv(
            R"(
              if ({1}::canCast(Kind)) {{
                return std::make_optional<{0}>({1}(std::move(Node)));
              }
              )",
            Record->getName(), Derivedname);
      });

  OS << llvm::formatv(
      R"(
      [[nodiscard]] static std::optional<{0}>
      cast(syntax::SyntaxNode Node) noexcept {
        const SyntaxKind Kind = static_cast<SyntaxKind>(Node.getKind());
        
        {1}
        
        return std::nullopt;
      }
      )",
      Record->getName(), llvm::join(IfStatements, "\n"));
}
} // namespace variant

std::string AstNodeGenerator::getIncludeGuardName() const noexcept {
  const llvm::StringRef Basename =
      llvm::sys::path::stem(Records_.getInputFilename());
  return "YUZU_AST_" + Basename.upper() + "_INC_H";
}

void AstNodeGenerator::emitClassDefinitions(
    llvm::raw_ostream &OS) const noexcept {
  (void)OS;

  for (const auto &[Name, Record] : Records_.getClasses()) {
    if (isIgnored(Record.get())) {
      continue;
    }

    OS << "class " << Name << ";\n";
  }

  for (const auto &[Name, Record] : Records_.getDefs()) {
    if (isIgnored(Record.get())) {
      continue;
    }

    OS << llvm::formatv(
        R"(
        class {0} final : public AstNode<{0}> {{
        public:
          explicit {0}(syntax::SyntaxNode Node) : AstNode(std::move(Node)) {{}

          {0}() = delete;

        )",
        Name);

    node::emitCanCast(OS, Record.get());
    node::emitCast(OS, Record.get());

    node::emitAstMethods(OS, Record.get());

    OS << "\n};\n\n";
  }

  for (const auto &[Name, Record] : Records_.getClasses()) {
    if (isIgnored(Record.get())) {
      continue;
    }

    const auto DerivedDefinitions =
        Records_.getAllDerivedDefinitionsIfDefined(Name);

    const std::string VariantClasses =
        llvm::join(llvm::map_range(DerivedDefinitions,
                                   [](const auto &Subclass) {
                                     return Subclass->getName();
                                   }),
                   ", ");

    OS << llvm::formatv(
        R"(
        class {0} : public std::variant<{1}> {{
        public:
          using std::variant<{1}>::variant;

          {0}() = delete;

        )",
        Name, VariantClasses);

    variant::emitCanCast(OS, Record.get(), DerivedDefinitions);
    variant::emitCast(OS, Record.get(), DerivedDefinitions);

    OS << "\n};\n\n";
  }
}

void AstNodeGenerator::emitHeader(llvm::raw_ostream &OS) const noexcept {
  emitSourceFileHeader("Yuzu AST Node Declarations", OS);

  emitOpenIncludeGuards(OS);
  emitIncludes(OS);

  OS << "namespace yuzu::ast {\n";

  emitClassDefinitions(OS);

  OS << "} // namespace yuzu::ast\n\n";

  emitCloseIncludeGuards(OS);
}

void AstNodeGenerator::runImpl(llvm::raw_ostream &OS) const noexcept {
  emitHeader(OS);
}
} // namespace yuzu_tools
