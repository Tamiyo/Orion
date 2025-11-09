#include "yuzu/tools/TableGen/AstNodeGenerator.h"

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
};
} // namespace

static llvm::cl::opt<ActionType>
    action(llvm::cl::desc("Action to perform:"),
           llvm::cl::values(clEnumValN(GenAstNodeDecls, "gen-ast-decls",
                                       "Generate AST node declarations")));

static bool YuzuTableGenMain(llvm::raw_ostream &os,
                             const llvm::RecordKeeper &Records) {
  switch (action) {
  case GenAstNodeDecls: {
    const auto projectIncludes = std::set<std::string>{
        "yuzu/Ast/Ast.h", "yuzu/Ast/Syntax.h", "yuzu/Syntax/Syntax.h",
        "yuzu/Util/ErrorHandling.h"};

    const auto externalIncludes = std::set<std::string>{};

    const auto systemIncludes =
        std::set<std::string>{"memory", "optional", "variant", "utility"};

    yuzu_tools::AstNodeGenerator(Records, projectIncludes, externalIncludes,
                                 systemIncludes)
        .run(os);
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
