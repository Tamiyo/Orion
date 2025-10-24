#include "llvm/ADT/StringRef.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/Path.h"
#include "llvm/TableGen/Main.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/TableGenBackend.h"

#include <set>
#include <string>

#define DEBUG_TYPE "yuzu-tblgen"

using namespace llvm;

namespace {
class YuzuEmitter {
public:
  explicit YuzuEmitter(const RecordKeeper &Records) : Records_(Records) {}

  void run(raw_ostream &OS);

private:
  void emitOpenIncludeGuards(raw_ostream &OS);
  void emitCloseIncludeGuards(raw_ostream &OS);
  void emitIncludes(raw_ostream &OS);
  void emitClassDefinitions(raw_ostream &OS);
  void emitClassMethods(raw_ostream &OS, const Record *const &Record);

  const RecordKeeper &Records_;
};
} // namespace

void YuzuEmitter::emitOpenIncludeGuards(raw_ostream &OS) {
  /// Get the input filename from the RecordKeeper.
  const StringRef InputFilename = Records_.getInputFilename();

  /// Extract just the filename without extension.
  /// e.g., "src/Ast/TableGen/Expr.td" -> "Expr".
  const StringRef Basename = llvm::sys::path::stem(InputFilename);

  const std::string GuardName = "YUZU_AST_" + Basename.upper() + "_INC_H";

  OS << "#ifndef " << GuardName << "\n";
  OS << "#define " << GuardName << "\n\n";
}

void YuzuEmitter::emitCloseIncludeGuards(raw_ostream &OS) {
  const StringRef Basename = llvm::sys::path::stem(Records_.getInputFilename());
  const std::string GuardName = "YUZU_AST_" + Basename.upper() + "_INC_H";
  OS << "#endif // YUZU_" << GuardName << "\n";
}

void YuzuEmitter::emitIncludes(raw_ostream &OS) {
  std::set<StringRef> ProjectIncludes{"Ast/Ast.h", "Syntax/Syntax.h"};

  std::set<StringRef> ExternalIncludes;

  std::set<StringRef> SystemIncludes{"memory", "optional", "variant"};

  const auto EmitIncludes = [&OS](std::set<StringRef> &Includes) {
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

void YuzuEmitter::emitClassDefinitions(raw_ostream &OS) {
  // Forward declare any abstract classes.
  for (const auto &[Name, Record] : Records_.getClasses()) {
    OS << "class " << Name << ";\n";
  }

  OS << "\n";

  // Emit the class defs.
  for (const auto &[Name, Record] : Records_.getDefs()) {
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

void YuzuEmitter::emitClassMethods(raw_ostream &OS,
                                   const Record *const &Record) {
  (void)OS;
  (void)Record;
}

void YuzuEmitter::run(raw_ostream &OS) {
  emitSourceFileHeader("Yuzu AST Node Declarations", OS);

  emitOpenIncludeGuards(OS);
  emitIncludes(OS);

  OS << "namespace yuzu::ast {\n";

  emitClassDefinitions(OS);

  OS << "} // namespace yuzu::ast\n\n";

  emitCloseIncludeGuards(OS);
}

namespace {
enum ActionType {
  GenAstDecls,
};
} // namespace

static cl::opt<ActionType>
    Action(cl::desc("Action to perform:"),
           cl::values(clEnumValN(GenAstDecls, "gen-ast-decls",
                                 "Generate AST declarations (headers)")));

static bool YuzuTableGenMain(raw_ostream &OS, const RecordKeeper &Records) {
  switch (Action) {
  case GenAstDecls:
    YuzuEmitter(Records).run(OS);
    break;
  }
  return false;
}

int main(int argc, char **argv) {
  InitLLVM X(argc, argv);
  cl::ParseCommandLineOptions(argc, argv, "Yuzu TableGen");

  return TableGenMain(argv[0], &YuzuTableGenMain);
}
