#include "TableGen/AstGenerator.h"
#include "yuzu/Tools/TableGen/AstNodeGenerator.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TableGen/Main.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/TableGenBackend.h"

#define DEBUG_TYPE "yuzu-tblgen"

namespace {
enum ActionType {
  GenAstNodeDecls,
  GenAstNodeDefs,
};
} // namespace

static llvm::cl::opt<ActionType>
    Action(llvm::cl::desc("Action to perform:"),
           llvm::cl::values(clEnumValN(GenAstNodeDecls, "gen-ast-decls",
                                       "Generate AST node declarations"),
                            clEnumValN(GenAstNodeDefs, "gen-ast-defs",
                                       "Generate AST node definitions")));

static bool YuzuTableGenMain(llvm::raw_ostream &OS,
                             const llvm::RecordKeeper &Records) {
  switch (Action) {
  case GenAstNodeDecls: {
    const auto ProjectIncludes = std::set<std::string>{
        "yuzu/Ast/Ast.h", "yuzu/Ast/SyntaxKind.h", "yuzu/Syntax/Syntax.h",
        "yuzu/Util/ErrorHandling.h"};

    const auto ExternalIncludes = std::set<std::string>{};

    const auto SystemIncludes = std::set<std::string>{"variant", "utility"};

    yuzu_tools::AstNodeGenerator(
        Records, yuzu_tools::AstGenerator::Type::Header, ProjectIncludes,
        ExternalIncludes, SystemIncludes)
        .run(OS);
    break;
  }

  case GenAstNodeDefs: {
    const auto ProjectIncludes = std::set<std::string>{
        "yuzu/Ast/Ast.h", "yuzu/Ast/SyntaxKind.h", "yuzu/Syntax/Syntax.h",
        "yuzu/Syntax/SyntaxIterator.h", "yuzu/Util/ErrorHandling.h"};

    const auto ExternalIncludes = std::set<std::string>{};

    const auto SystemIncludes =
        std::set<std::string>{"memory", "optional", "string", "variant"};

    yuzu_tools::AstNodeGenerator(
        Records, yuzu_tools::AstGenerator::Type::Source, ProjectIncludes,
        ExternalIncludes, SystemIncludes)
        .run(OS);
    break;
  }
  }

  return false;
}

int main(int argc, char **argv) {
  llvm::InitLLVM X(argc, argv);

  llvm::cl::ParseCommandLineOptions(argc, argv, "Yuzu TableGen");
  return TableGenMain(argv[0], &YuzuTableGenMain);
}
