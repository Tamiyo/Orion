#include "AstNodeGenerator.h"
#include "HirBuilderGenerator.h"
#include "HirKindGenerator.h"
#include "HirNodeGenerator.h"
#include "HirPrinterGenerator.h"
#include "HirVisitorGenerator.h"
#include "SyntaxKindGenerator.h"
#include "TextMateGrammarGenerator.h"
#include "TokenKindGenerator.h"

#include <llvm/Support/CommandLine.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/TableGen/Main.h>
#include <llvm/TableGen/Record.h>

namespace {
enum ActionType {
  GenAstNodeDecls,
  GenHirBuilderDecls,
  GenHirKindDecls,
  GenHirNodeDecls,
  GenHirPrinterDecls,
  GenHirVisitorDecls,
  GenAnfBuilderDecls,
  GenAnfKindDecls,
  GenAnfNodeDecls,
  GenAnfVisitorDecls,
  GenSyntaxKindDecls,
  GenTokenKindDecls,
  GenTextMateGrammar,
};
} // namespace

static llvm::cl::opt<ActionType> action(
    llvm::cl::desc("Action to perform:"),
    llvm::cl::values(clEnumValN(GenAstNodeDecls, "gen-ast-node-decls",
                                "Generate AST node and variant declarations")),
    llvm::cl::values(clEnumValN(GenHirBuilderDecls, "gen-hir-builder-decls",
                                "Generate HirBuilder declarations")),
    llvm::cl::values(clEnumValN(GenHirKindDecls, "gen-hir-kind-decls",
                                "Generate HirKind declarations")),
    llvm::cl::values(clEnumValN(GenHirNodeDecls, "gen-hir-node-decls",
                                "Generate HIR node and variant declarations")),
    llvm::cl::values(clEnumValN(GenHirPrinterDecls, "gen-hir-printer-decls",
                                "Generate HIR pretty-printer declarations")),
    llvm::cl::values(clEnumValN(GenHirVisitorDecls, "gen-hir-visitor-decls",
                                "Generate HIR visitor declarations")),
    llvm::cl::values(clEnumValN(GenAnfBuilderDecls, "gen-anf-builder-decls",
                                "Generate AnfBuilder declarations")),
    llvm::cl::values(clEnumValN(GenAnfKindDecls, "gen-anf-kind-decls",
                                "Generate AnfKind declarations")),
    llvm::cl::values(clEnumValN(GenAnfNodeDecls, "gen-anf-node-decls",
                                "Generate ANF node and variant declarations")),
    llvm::cl::values(clEnumValN(GenAnfVisitorDecls, "gen-anf-visitor-decls",
                                "Generate ANF visitor declarations")),
    llvm::cl::values(clEnumValN(GenSyntaxKindDecls, "gen-syntax-kind-decls",
                                "Generate SyntaxKind declarations")),
    llvm::cl::values(clEnumValN(GenTokenKindDecls, "gen-token-kind-decls",
                                "Generate TokenKind declarations")),
    llvm::cl::values(clEnumValN(GenTextMateGrammar, "gen-textmate-grammar",
                                "Generate the TextMate grammar (JSON)")));

static bool YuzuTableGenMain(llvm::raw_ostream &os,
                             const llvm::RecordKeeper &records) {
  switch (action) {
  case GenAstNodeDecls:
    yuzu::tools::AstNodeGenerator(os).run(records);
    break;
  case GenHirBuilderDecls:
    yuzu::tools::HirBuilderGenerator(os).run(records);
    break;
  case GenHirKindDecls:
    yuzu::tools::HirKindGenerator(os).run(records);
    break;
  case GenHirNodeDecls:
    yuzu::tools::HirNodeGenerator(os).run(records);
    break;
  case GenHirPrinterDecls:
    yuzu::tools::HirPrinterGenerator(os).run(records);
    break;
  case GenHirVisitorDecls:
    yuzu::tools::HirVisitorGenerator(os).run(records);
    break;
  // The Hir* node/kind/builder/visitor generators are tree-agnostic — they read
  // the class-name prefix from each schema's `TreeName`. ANF reuses them to
  // emit `Anf*` outputs (only the HIR *printer* stays HIR-specific). A future
  // cleanup could rename these generator classes to drop the `Hir` prefix.
  case GenAnfBuilderDecls:
    yuzu::tools::HirBuilderGenerator(os).run(records);
    break;
  case GenAnfKindDecls:
    yuzu::tools::HirKindGenerator(os).run(records);
    break;
  case GenAnfNodeDecls:
    yuzu::tools::HirNodeGenerator(os).run(records);
    break;
  case GenAnfVisitorDecls:
    yuzu::tools::HirVisitorGenerator(os).run(records);
    break;
  case GenSyntaxKindDecls:
    yuzu::tools::SyntaxKindGenerator(os).run(records);
    break;
  case GenTokenKindDecls:
    yuzu::tools::TokenKindGenerator(os).run(records);
    break;
  case GenTextMateGrammar:
    yuzu::tools::TextMateGrammarGenerator(os).run(records);
    break;
  }

  return false;
}

int main(int argc, char **argv) {
  llvm::InitLLVM X(argc, argv);

  llvm::cl::ParseCommandLineOptions(argc, argv, "Yuzu TableGen");
  return TableGenMain(argv[0], &YuzuTableGenMain);
}
