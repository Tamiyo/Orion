#include "yuzu/Tools/TableGen/AstBuilderGenerator.h"

#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TableGen/Main.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/TableGenBackend.h"

namespace yuzu_tools {
std::string AstBuilderGenerator::getIncludeGuardName() const {
  const llvm::StringRef Basename =
      llvm::sys::path::stem(Records_.getInputFilename());
  return "YUZU_AST_" + Basename.upper() + "BUILDER_INC_H";
}

void AstBuilderGenerator::emitClassDefinitions(llvm::raw_ostream &OS) const {
  const llvm::StringRef Basename =
      llvm::sys::path::stem(Records_.getInputFilename());

  OS << llvm::formatv(R"(// Forward Declarations.
class {0};

// Implementation.
class {0}Builder final {{
public:
  [[nodiscard]] static std::unique_ptr<{0}>
  tryFrom(const syntax::SyntaxNode &Node);
};
)",
  Basename);
}

void AstBuilderGenerator::emitClassMethods(llvm::raw_ostream &OS,
                                           const llvm::Record *Record) const {
  const llvm::StringRef Basename =
      llvm::sys::path::stem(Records_.getInputFilename());

  OS << llvm::formatv(R"(
std::unique_ptr<{0}> {0}Builder::tryFrom(const syntax::SyntaxNode &Node) {{
  switch(static_cast<SyntaxKind>(Node.getKind())) {{
)",
  Basename);

  for (const auto &DerivedRecord :
       Records_.getAllDerivedDefinitions(Record->getName())) {
    OS << llvm::formatv(R"(
  case SyntaxKind::{0}:
    return std::make_unique<Expr>(std::in_place_type<{1}>, Node);
)",
    DerivedRecord->getValueAsString("SyntaxKind"),
    DerivedRecord->getName());
  }

  OS << R"(
  default:
    break;
  }

  return nullptr;
}
)";
}

void AstBuilderGenerator::emitHeader(llvm::raw_ostream &OS) const {
  emitSourceFileHeader("Yuzu AST Builder Declarations", OS);

  emitOpenIncludeGuards(OS);
  emitIncludes(OS);

  OS << "namespace yuzu::ast {\n";

  emitClassDefinitions(OS);

  OS << "} // namespace yuzu::ast\n";

  emitCloseIncludeGuards(OS);
}

void AstBuilderGenerator::emitSource(llvm::raw_ostream &OS) const {
  const llvm::StringRef Basename =
      llvm::sys::path::stem(Records_.getInputFilename());

  emitSourceFileHeader("Yuzu AST Builder Definitions", OS);

  OS << llvm::formatv(R"(
// Include main header.
#include "yuzu/Ast/{0}Builder.h.inc"

// Append secondary header.
#include "yuzu/Ast/{0}.h"
)",
  Basename);

  emitIncludes(OS);

  OS << "namespace yuzu::ast {\n";

  emitClassMethods(OS, Records_.getClass(Basename));

  OS << "} // namespace yuzu::ast\n";
}

void AstBuilderGenerator::runImpl(llvm::raw_ostream &OS) const {
  if (Type_ == AstGenerator::Type::Source) {
    emitSource(OS);
  } else {
    emitHeader(OS);
  }
}
} // namespace yuzu_tools
