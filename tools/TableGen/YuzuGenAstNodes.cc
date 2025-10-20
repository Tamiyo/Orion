#include "llvm/Support/CommandLine.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/TableGen/Main.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/TableGenBackend.h"

using namespace llvm;

namespace {
class YuzuEmitter {
public:
  explicit YuzuEmitter(const RecordKeeper &Records) : Records_(Records) {}

  void run(raw_ostream &OS);

private:
  const RecordKeeper &Records_;
};
} // namespace

void YuzuEmitter::run(raw_ostream &OS) {
  emitSourceFileHeader("Yuzu AST Node Declarations", OS);

  (void)Records_;
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
