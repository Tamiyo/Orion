#include "TableGen/YuzuAstGenerator.h"

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
  GenAstDecls,
};
} // namespace

static llvm::cl::opt<ActionType>
    Action(llvm::cl::desc("Action to perform:"),
           llvm::cl::values(clEnumValN(GenAstDecls, "gen-ast-decls",
                                       "Generate AST declarations (headers)")));

static bool YuzuTableGenMain(llvm::raw_ostream &OS,
                             const llvm::RecordKeeper &Records) {
  switch (Action) {
  case GenAstDecls:
    yuzu_tools::YuzuAstGenerator(Records).run(OS);
    break;
  }
  return false;
}

int main(int argc, char **argv) {
  llvm::InitLLVM X(argc, argv);

  llvm::cl::ParseCommandLineOptions(argc, argv, "Yuzu TableGen");
  return TableGenMain(argv[0], &YuzuTableGenMain);
}
