#include "TableGen/AstGenerator.h"
#include "yuzu/Tools/TableGen/AstBuilderGenerator.h"
#include "yuzu/Tools/TableGen/AstNodeGenerator.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TableGen/Main.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/TableGenBackend.h"

#include <set>
#include <string>

#define DEBUG_TYPE "yuzu-tblgen"

namespace {
enum ActionType {
  GenAstNodeDecls,
  GenAstNodeDefs,
  GenAstBuilderDecls,
  GenAstBuilderDefs,
};
} // namespace

static llvm::cl::opt<ActionType> Action(
    llvm::cl::desc("Action to perform:"),
    llvm::cl::values(clEnumValN(GenAstNodeDecls, "gen-ast-decls",
                                "Generate AST node declarations"),
                     clEnumValN(GenAstNodeDefs, "gen-ast-defs",
                                "Generate AST node definitions"),
                     clEnumValN(GenAstBuilderDecls, "gen-ast-builder-decls",
                                "Generate AST builder declarations"),
                     clEnumValN(GenAstBuilderDefs, "gen-ast-builder-defs",
                                "Generate AST builder definitions")));

static bool YuzuTableGenMain(llvm::raw_ostream &OS,
                             const llvm::RecordKeeper &Records) {
  switch (Action) {
  case GenAstNodeDecls: {
    const auto ProjectIncludes = std::set<std::string>{
        "yuzu/Ast/ExprBuilder.h.inc", "yuzu/Ast/Ast.h",
        "yuzu/Syntax/Syntax.h", "yuzu/Syntax/SyntaxIterator.h"};

    const auto ExternalIncludes = std::set<std::string>{};

    const auto SystemIncludes = std::set<std::string>{"memory", "variant"};

    yuzu_tools::AstNodeGenerator(
        Records, yuzu_tools::AstGenerator::Type::Header, ProjectIncludes,
        ExternalIncludes, SystemIncludes)
        .run(OS);
    break;
  }

  case GenAstNodeDefs: {
    const auto ProjectIncludes = std::set<std::string>{
        "yuzu/Ast/Ast.h", "yuzu/Syntax/Syntax.h",
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

  case GenAstBuilderDecls: {
    const auto ProjectIncludes = std::set<std::string>{"yuzu/Syntax/Syntax.h"};
    const auto ExternalIncludes = std::set<std::string>{};
    const auto SystemIncludes = std::set<std::string>{"memory"};

    yuzu_tools::AstBuilderGenerator(
        Records, yuzu_tools::AstGenerator::Type::Header, ProjectIncludes,
        ExternalIncludes, SystemIncludes)
        .run(OS);
    break;
  }

  case GenAstBuilderDefs: {
    const auto ProjectIncludes =
        std::set<std::string>{"yuzu/Ast/SyntaxKind.h", "yuzu/Syntax/Syntax.h"};

    const auto ExternalIncludes = std::set<std::string>{};
    const auto SystemIncludes = std::set<std::string>{"memory", "utility"};

    yuzu_tools::AstBuilderGenerator(
        Records, yuzu_tools::AstGenerator::Type::Source, ProjectIncludes,
        ExternalIncludes, SystemIncludes)
        .run(OS);
  } break;
  }

  return false;
}

int main(int argc, char **argv) {
  llvm::InitLLVM X(argc, argv);

  llvm::cl::ParseCommandLineOptions(argc, argv, "Yuzu TableGen");
  return TableGenMain(argv[0], &YuzuTableGenMain);
}
