#include "TableGen/YuzuAstGenerator.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TableGen/Main.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/TableGenBackend.h"

#include <set>
#include <string>

namespace {
const std::set<std::string> IgnoredClasses = {
    "AstNode",
    "AstMethod",
};
}

namespace yuzu_tools {
void YuzuAstGenerator::emitOpenIncludeGuards(llvm::raw_ostream &OS) {
  /// Get the input filename from the RecordKeeper.
  const llvm::StringRef InputFilename = Records_.getInputFilename();

  /// Extract just the filename without extension.
  /// e.g., "yuzu/Ast/TableGen/Expr.td" -> "Expr".
  const llvm::StringRef Basename = llvm::sys::path::stem(InputFilename);
  const std::string GuardName = "YUZU_AST_" + Basename.upper() + "_INC_H";

  OS << "#ifndef " << GuardName << "\n";
  OS << "#define " << GuardName << "\n\n";
}

void YuzuAstGenerator::emitCloseIncludeGuards(llvm::raw_ostream &OS) {
  const llvm::StringRef Basename =
      llvm::sys::path::stem(Records_.getInputFilename());

  const std::string GuardName = "YUZU_AST_" + Basename.upper() + "_INC_H";

  OS << "#endif // YUZU_" << GuardName << "\n";
}

void YuzuAstGenerator::emitIncludes(llvm::raw_ostream &OS) {
  const std::set<llvm::StringRef> ProjectIncludes{"yuzu/Ast/Ast.h",
                                                  "yuzu/Syntax/Syntax.h"};

  const std::set<llvm::StringRef> ExternalIncludes;

  const std::set<llvm::StringRef> SystemIncludes{"memory", "optional",
                                                 "variant"};

  const auto EmitIncludes = [&OS](const std::set<llvm::StringRef> &Includes) {
    if (Includes.empty()) {
      return;
    }

    for (const auto &Include : Includes) {
      if (Include.find(".h") != std::string::npos) {
        OS << "#include \"" << Include << "\"\n";
      } else {
        OS << "#include <" << Include << ">\n";
      }
    }
    OS << "\n";
  };

  /// Emit includes using LLVM style.
  EmitIncludes(ProjectIncludes);
  EmitIncludes(ExternalIncludes);
  EmitIncludes(SystemIncludes);
}

void YuzuAstGenerator::emitClassDefinitions(llvm::raw_ostream &OS) {
  // Forward declare any abstract classes.
  for (const auto &[Name, Record] : Records_.getClasses()) {
    // Skip any classes that should be ignored.
    if (IgnoredClasses.find(Name) != IgnoredClasses.end()) {
      continue;
    }

    OS << "class " << Name << ";\n";
  }

  OS << "\n";

  // Emit the class defs.
  for (const auto &[Name, Record] : Records_.getDefs()) {
    // Skip any classes that should be ignored.
    if (IgnoredClasses.find(Name) != IgnoredClasses.end()) {
      continue;
    }

    OS << "class " << Name << " final : public AstNode {\n";
    OS << "public:\n";
    OS << "  explicit " << Name
       << "(syntax::SyntaxNode Node) : AstNode(std::move(Node)) {}\n\n";

    OS << "  " << Name << "() = delete;\n";
    OS << "  " << Name << "(const " << Name << " &) = delete;\n";
    OS << "  " << Name << " &operator=(const " << Name << " &) = delete;\n";

    // OS << "\n";
    // emitClassMethods(OS);
    // OS << "\n";

    OS << "};\n\n";
  }

  // Emit the abstract class defs as implementations of std::variant.
  for (const auto &[Name, Record] : Records_.getClasses()) {
    // Skip any classes that should be ignored.
    if (IgnoredClasses.find(Name) != IgnoredClasses.end()) {
      continue;
    }

    const auto Subclasses = Records_.getAllDerivedDefinitionsIfDefined(Name);

    OS << "class " << Name << " : public std::variant<";
    for (size_t i = 0, s = Subclasses.size(); i < s; i++) {
      const auto &Subclass = Subclasses[i];
      OS << Subclass->getName();
      if (i != s - 1) {
        OS << ", ";
      }
    }
    OS << "> {\n";

    OS << "  using std::variant<";
    for (size_t i = 0, s = Subclasses.size(); i < s; i++) {
      const auto &Subclass = Subclasses[i];
      OS << Subclass->getName();
      if (i != s - 1) {
        OS << ", ";
      }
    }
    OS << ">::variant;\n\n";

    OS << "  " << Name << "() = delete;\n";
    OS << "};\n";
  }

  (void)OS;
}

void YuzuAstGenerator::emitClassMethods(llvm::raw_ostream &OS,
                                        const llvm::Record *const &Record) {
  (void)OS;
  (void)Record;
}

void YuzuAstGenerator::runImpl(llvm::raw_ostream &OS) {
  emitSourceFileHeader("Yuzu AST Node Declarations", OS);

  emitOpenIncludeGuards(OS);
  emitIncludes(OS);

  OS << "namespace yuzu::ast {\n";

  emitClassDefinitions(OS);

  OS << "} // namespace yuzu::ast\n\n";

  emitCloseIncludeGuards(OS);
}
} // namespace yuzu_tools
