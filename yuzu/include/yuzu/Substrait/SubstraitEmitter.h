#ifndef YUZU_SUBSTRAIT_SUBSTRAITEMITTER_H
#define YUZU_SUBSTRAIT_SUBSTRAITEMITTER_H

#include "yuzu/Anf/Anf.h"

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/JSON.h>

#include <string>
#include <vector>

namespace yuzu::substrait {

/// Emits a Substrait plan as JSON (for DuckDB's `from_substrait_json`) from a
/// fully reduced ANF program. The relational layer maps directly —
/// `SelectRel` -> `ProjectRel`, `FromRel` -> `ReadRel` — and each column's
/// `Thunk` is inlined into one scalar `Expression` (constants -> `literal`,
/// row fields -> `selection`, builtin calls -> `scalarFunction`). Scalar
/// functions used are collected into the plan's extension declarations.
class SubstraitEmitter final {
public:
  /// The plan as pretty-printed JSON; empty if the program has no query.
  std::string emit(const anf::Root *root);

private:
  /// A column's let-bindings, so a `VarAtom` use inlines its defining value
  /// while building the expression tree.
  using Env = llvm::DenseMap<const anf::Binding *, const anf::Expr *>;

  llvm::json::Value emitRel(const anf::Rel *rel);
  llvm::json::Value emitFromRel(const anf::FromRel *from);
  llvm::json::Value emitSelectRel(const anf::SelectRel *select);
  llvm::json::Value emitWhereRel(const anf::WhereRel *where);
  llvm::json::Value emitDistinctRel(const anf::DistinctRel *distinct);
  llvm::json::Value emitDropRel(const anf::DropRel *drop);
  llvm::json::Value emitRenameRel(const anf::RenameRel *rename);
  llvm::json::Value emitThunk(const anf::Thunk *thunk);

  llvm::json::Value emitExpr(const anf::Expr *expr, const Env &env);
  llvm::json::Value emitAtom(const anf::Atom *atom, const Env &env);
  llvm::json::Value emitLiteral(const anf::Constant *constant);
  llvm::json::Value emitScalarFunction(const anf::CallExpr *call,
                                       const Env &env);
  llvm::json::Value emitSelection(const anf::FieldAtom *field);

  /// Intern a `(uri, name)` function into the extension tables; returns its
  /// function anchor.
  int registerFunction(llvm::StringRef uri, llvm::StringRef name);
  llvm::json::Value emitExtensionUris() const;
  llvm::json::Value emitExtensions() const;

  // Extension tables, built as functions are registered.
  llvm::DenseMap<llvm::StringRef, int> uriAnchors;
  std::vector<std::string> uris;
  struct Function {
    int uriAnchor;
    int functionAnchor;
    std::string name;
  };
  std::vector<Function> functions;
};

} // namespace yuzu::substrait

#endif // YUZU_SUBSTRAIT_SUBSTRAITEMITTER_H
