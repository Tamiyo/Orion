#include "yuzu/Substrait/SubstraitEmitter.h"

#include "yuzu/Anf/Anf.h"
#include "yuzu/Ops/BuiltinOp.h"
#include "yuzu/Types/Type.h"
#include "yuzu/Util/ErrorHandling.h"
#include "yuzu/Util/Unicode.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/raw_ostream.h>

#include <optional>
#include <string>

namespace yuzu::substrait {

using llvm::json::Array;
using llvm::json::Object;
using llvm::json::Value;

namespace {

// Substrait standard extension YAMLs (the function families DuckDB consumes).
constexpr llvm::StringRef kArithmeticUri =
    "https://github.com/substrait-io/substrait/blob/main/extensions/"
    "functions_arithmetic.yaml";
constexpr llvm::StringRef kComparisonUri =
    "https://github.com/substrait-io/substrait/blob/main/extensions/"
    "functions_comparison.yaml";
constexpr llvm::StringRef kBooleanUri =
    "https://github.com/substrait-io/substrait/blob/main/extensions/"
    "functions_boolean.yaml";

// Every type is emitted non-null for now; nullability is future work.
constexpr llvm::StringRef kNullability = "NULLABILITY_REQUIRED";

/// The Substrait type code for a scalar yuzu type (`i32`, `fp64`, ...). Used
/// both in type objects and in function-signature suffixes.
llvm::StringRef typeCode(const types::Type *type) {
  switch (type->getKind()) {
  case types::TypeKind::Int8:
  case types::TypeKind::UInt8:
    return "i8";
  case types::TypeKind::Int16:
  case types::TypeKind::UInt16:
    return "i16";
  case types::TypeKind::Int32:
  case types::TypeKind::UInt32:
    return "i32";
  case types::TypeKind::Int64:
  case types::TypeKind::UInt64:
    return "i64";
  case types::TypeKind::Float32:
    return "fp32";
  case types::TypeKind::Float64:
    return "fp64";
  case types::TypeKind::Bool:
    return "bool";
  case types::TypeKind::Str:
    return "string";
  default:
    break;
  }
  util::yuzu_unreachable("no Substrait type code for this yuzu type");
}

/// A Substrait type object, e.g. `{"i32": {"nullability": "..._REQUIRED"}}`.
Value emitType(const types::Type *type) {
  return Object{{typeCode(type), Object{{"nullability", kNullability}}}};
}

/// A Substrait field selection by column index over the current input row.
Value selectionByIndex(int index) {
  return Object{
      {"selection", Object{{"directReference",
                            Object{{"structField", Object{{"field", index}}}}},
                           {"rootReference", Object{}}}}};
}

struct FunctionTarget {
  llvm::StringRef uri;
  llvm::StringRef base; // Substrait function name, e.g. "add".
};

/// Map an ANF builtin operator to its Substrait extension function. An operator
/// with no Substrait equivalent yet (e.g. `**`, `in`, unary `+`) returns null.
std::optional<FunctionTarget> functionTarget(BuiltinOp op) {
  switch (op) {
  case BuiltinOp::Add:
    return FunctionTarget{kArithmeticUri, "add"};
  case BuiltinOp::Sub:
    return FunctionTarget{kArithmeticUri, "subtract"};
  case BuiltinOp::Mul:
    return FunctionTarget{kArithmeticUri, "multiply"};
  case BuiltinOp::Div:
    return FunctionTarget{kArithmeticUri, "divide"};
  case BuiltinOp::ShiftLeft:
    return FunctionTarget{kArithmeticUri, "shift_left"};
  case BuiltinOp::ShiftRight:
    return FunctionTarget{kArithmeticUri, "shift_right"};
  case BuiltinOp::UnaryNeg:
    return FunctionTarget{kArithmeticUri, "negate"};
  case BuiltinOp::Eq:
    return FunctionTarget{kComparisonUri, "equal"};
  case BuiltinOp::Neq:
    return FunctionTarget{kComparisonUri, "not_equal"};
  case BuiltinOp::Lt:
    return FunctionTarget{kComparisonUri, "lt"};
  case BuiltinOp::Lte:
    return FunctionTarget{kComparisonUri, "lte"};
  case BuiltinOp::Gt:
    return FunctionTarget{kComparisonUri, "gt"};
  case BuiltinOp::Gte:
    return FunctionTarget{kComparisonUri, "gte"};
  case BuiltinOp::And:
    return FunctionTarget{kBooleanUri, "and"};
  case BuiltinOp::Or:
    return FunctionTarget{kBooleanUri, "or"};
  case BuiltinOp::UnaryNot:
    return FunctionTarget{kBooleanUri, "not"};
  case BuiltinOp::Pow:
  case BuiltinOp::In:
  case BuiltinOp::NotIn:
  case BuiltinOp::UnaryPos:
    return std::nullopt;
  }
  return std::nullopt;
}

/// Index of `name` within a struct type's fields (the Substrait field offset),
/// or -1 if absent.
int structIndex(const types::Type *type, std::u32string_view name) {
  const auto *structType = types::StructType::cast(type);
  if (structType == nullptr) {
    return -1;
  }
  int index = 0;
  for (const types::StructField &field : structType->getFields()) {
    if (field.name == name) {
      return index;
    }
    ++index;
  }
  return -1;
}

/// The row struct a relation yields (`Relation[Row]` -> `Row`).
const types::StructType *rowStruct(const types::Type *relationType) {
  const auto *relation = types::RelationType::cast(relationType);
  if (relation == nullptr) {
    return nullptr;
  }
  return types::StructType::cast(relation->getElement());
}

} // namespace

int SubstraitEmitter::registerFunction(llvm::StringRef uri,
                                       llvm::StringRef name) {
  const auto [it, inserted] =
      uriAnchors.try_emplace(uri, static_cast<int>(uris.size()) + 1);
  if (inserted) {
    uris.push_back(uri.str());
  }
  const int functionAnchor = static_cast<int>(functions.size()) + 1;
  functions.push_back({it->second, functionAnchor, name.str()});
  return functionAnchor;
}

llvm::json::Value SubstraitEmitter::emitExtensionUris() const {
  Array out;
  for (std::size_t i = 0; i < uris.size(); ++i) {
    out.push_back(Object{{"extensionUriAnchor", static_cast<int>(i) + 1},
                         {"uri", uris[i]}});
  }
  return out;
}

llvm::json::Value SubstraitEmitter::emitExtensions() const {
  Array out;
  for (const Function &func : functions) {
    out.push_back(Object{
        {"extensionFunction", Object{{"extensionUriReference", func.uriAnchor},
                                     {"functionAnchor", func.functionAnchor},
                                     {"name", func.name}}}});
  }
  return out;
}

llvm::json::Value SubstraitEmitter::emitLiteral(const anf::Constant *constant) {
  Object literal;
  if (const auto *node = anf::IntConst::cast(constant)) {
    // Substrait JSON encodes i64 as a string, narrower ints as numbers.
    if (typeCode(node->getType()) == "i64") {
      literal["i64"] = std::to_string(node->getValue());
    } else {
      literal[typeCode(node->getType())] =
          static_cast<int64_t>(node->getValue());
    }
  } else if (const auto *node = anf::FloatConst::cast(constant)) {
    literal[typeCode(node->getType())] = node->getValue();
  } else if (const auto *node = anf::BoolConst::cast(constant)) {
    literal["boolean"] = node->getValue();
  } else if (const auto *node = anf::StringConst::cast(constant)) {
    literal["string"] = util::toUtf8(node->getValue());
  }
  return Object{{"literal", std::move(literal)}};
}

llvm::json::Value SubstraitEmitter::emitSelection(const anf::FieldAtom *field) {
  // A row field reference: its offset within the input's row struct.
  const int index =
      structIndex(field->getBase()->getType(), field->getField()->getName());
  if (index < 0) {
    util::yuzu_unreachable("field not found in row struct while emitting "
                           "Substrait selection");
  }
  return selectionByIndex(index);
}

llvm::json::Value
SubstraitEmitter::emitScalarFunction(const anf::CallExpr *call,
                                     const Env &env) {
  const auto target = functionTarget(call->getOp());
  if (!target) {
    util::yuzu_unreachable("no Substrait mapping for ANF operator");
  }

  // Compound name carries the operand type signature, e.g. "add:i32_i32".
  std::string name = target->base.str() + ":";
  Array arguments;
  bool first = true;
  for (const anf::Atom *arg : call->getArgs()) {
    if (!first) {
      name += "_";
    }
    first = false;
    name += typeCode(arg->getType()).str();
    arguments.push_back(Object{{"value", emitExpr(arg, env)}});
  }

  const int anchor = registerFunction(target->uri, name);
  return Object{
      {"scalarFunction", Object{{"functionReference", anchor},
                                {"outputType", emitType(call->getType())},
                                {"arguments", std::move(arguments)}}}};
}

llvm::json::Value SubstraitEmitter::emitExpr(const anf::Expr *expr,
                                             const Env &env) {
  switch (expr->getExprKind()) {
  case anf::ExprKind::Atom:
    return emitAtom(anf::Atom::cast(expr), env);
  case anf::ExprKind::CallExpr:
    return emitScalarFunction(anf::CallExpr::cast(expr), env);
  case anf::ExprKind::FuncCallExpr:
    util::yuzu_unreachable("FuncCallExpr should have been inlined before emit");
  case anf::ExprKind::StructExpr:
    util::yuzu_unreachable(
        "struct literals are not yet supported in Substrait");
  case anf::ExprKind::Rel:
    util::yuzu_unreachable("a relation cannot appear as a column expression");
  }
  util::yuzu_unreachable("unhandled expression kind while emitting Substrait");
}

llvm::json::Value SubstraitEmitter::emitAtom(const anf::Atom *atom,
                                             const Env &env) {
  switch (atom->getAtomKind()) {
  case anf::AtomKind::Constant:
    return emitLiteral(anf::Constant::cast(atom));
  case anf::AtomKind::VarAtom: {
    // Inline the column temporary this name binds (chase the def-use edge).
    const auto *var = anf::VarAtom::cast(atom);
    const auto it = env.find(var->getBinding());
    if (it == env.end()) {
      util::yuzu_unreachable("unbound VarAtom while emitting Substrait");
    }
    return emitExpr(it->second, env);
  }
  case anf::AtomKind::FieldAtom:
    return emitSelection(anf::FieldAtom::cast(atom));
  case anf::AtomKind::FuncRef:
    util::yuzu_unreachable(
        "a function reference cannot be emitted as a column");
  }
  util::yuzu_unreachable("unhandled atom kind while emitting Substrait");
}

llvm::json::Value SubstraitEmitter::emitThunk(const anf::Thunk *thunk) {
  // Inline the thunk's let-bindings into a single expression tree (a column
  // value or a `where` predicate).
  Env env;
  const anf::Expr *tail = nullptr;
  for (const anf::Stmt *stmt : thunk->getStmts()) {
    if (const auto *let = anf::LetStmt::cast(stmt)) {
      env[let->getBinding()] = let->getBinding()->getValue();
    } else if (const auto *exprStmt = anf::ExprStmt::cast(stmt)) {
      tail = exprStmt->getValue();
    }
  }
  return emitExpr(tail, env);
}

llvm::json::Value SubstraitEmitter::emitFromRel(const anf::FromRel *from) {
  const types::StructType *row = rowStruct(from->getType());

  Array names;
  Array types;
  if (row != nullptr) {
    for (const types::StructField &field : row->getFields()) {
      names.push_back(util::toUtf8(field.name));
      types.push_back(emitType(field.type));
    }
  }

  return Object{
      {"read",
       Object{{"baseSchema",
               Object{{"names", std::move(names)},
                      {"struct", Object{{"types", std::move(types)},
                                        {"nullability", kNullability}}}}},
              {"namedTable",
               Object{{"names", Array{util::toUtf8(
                                    from->getRelation()->getName())}}}}}}};
}

llvm::json::Value
SubstraitEmitter::emitSelectRel(const anf::SelectRel *select) {
  Value input = emitRel(anf::Rel::cast(select->getInput()));

  // Output keeps only the projected expressions, which Substrait appends after
  // the input's columns — so the emit mapping starts past them.
  const types::StructType *inputRow = rowStruct(select->getInput()->getType());
  const int inputColumns =
      inputRow != nullptr ? static_cast<int>(inputRow->getFields().size()) : 0;

  Array expressions;
  Array outputMapping;
  int output = inputColumns;
  for (const anf::SelectItem *item : select->getItems()) {
    expressions.push_back(emitThunk(item->getBody()));
    outputMapping.push_back(output++);
  }

  return Object{
      {"project",
       Object{{"common", Object{{"emit", Object{{"outputMapping",
                                                 std::move(outputMapping)}}}}},
              {"input", std::move(input)},
              {"expressions", std::move(expressions)}}}};
}

llvm::json::Value SubstraitEmitter::emitWhereRel(const anf::WhereRel *where) {
  // A `FilterRel` preserves the input's columns, so no `emit` mapping needed.
  Value input = emitRel(anf::Rel::cast(where->getInput()));
  Value condition = emitThunk(where->getPredicate());
  return Object{{"filter", Object{{"input", std::move(input)},
                                  {"condition", std::move(condition)}}}};
}

llvm::json::Value
SubstraitEmitter::emitDistinctRel(const anf::DistinctRel *distinct) {
  // Dedup is an aggregate grouping by every column with no measures, so the
  // output is the distinct combinations of all columns (in input order).
  Value input = emitRel(anf::Rel::cast(distinct->getInput()));

  const types::StructType *row = rowStruct(distinct->getType());
  Array groupingExpressions;
  if (row != nullptr) {
    for (int i = 0; i < static_cast<int>(row->getFields().size()); ++i) {
      groupingExpressions.push_back(selectionByIndex(i));
    }
  }

  return Object{
      {"aggregate",
       Object{{"input", std::move(input)},
              {"groupings", Array{Object{{"groupingExpressions",
                                          std::move(groupingExpressions)}}}},
              {"measures", Array{}}}}};
}

llvm::json::Value SubstraitEmitter::emitDropRel(const anf::DropRel *drop) {
  Value input = emitRel(anf::Rel::cast(drop->getInput()));

  // Project the survivors: each input column whose name isn't dropped, by its
  // input index.
  const types::StructType *inputRow = rowStruct(drop->getInput()->getType());
  Array outputMapping;
  if (inputRow != nullptr) {
    int index = 0;
    for (const types::StructField &field : inputRow->getFields()) {
      bool dropped = false;
      for (const anf::Ident *column : drop->getColumns()) {
        if (column->getName() == field.name) {
          dropped = true;
          break;
        }
      }
      if (!dropped) {
        outputMapping.push_back(index);
      }
      ++index;
    }
  }

  return Object{
      {"project",
       Object{{"common", Object{{"emit", Object{{"outputMapping",
                                                 std::move(outputMapping)}}}}},
              {"input", std::move(input)},
              {"expressions", Array{}}}}};
}

llvm::json::Value
SubstraitEmitter::emitRenameRel(const anf::RenameRel *rename) {
  // Rename is positionally a no-op: the new names live in the output row type
  // and surface as the plan's output `names`, so emit the input directly.
  return emitRel(anf::Rel::cast(rename->getInput()));
}

llvm::json::Value
SubstraitEmitter::emitExtendRel(const anf::ExtendRel *extend) {
  Value input = emitRel(anf::Rel::cast(extend->getInput()));

  // Append the new columns after the input's, and emit all of them: the input
  // columns by index, then the new expressions.
  const types::StructType *inputRow = rowStruct(extend->getInput()->getType());
  const int inputColumns =
      inputRow != nullptr ? static_cast<int>(inputRow->getFields().size()) : 0;

  Array expressions;
  Array outputMapping;
  for (int i = 0; i < inputColumns; ++i) {
    outputMapping.push_back(i);
  }
  int output = inputColumns;
  for (const anf::SelectItem *item : extend->getItems()) {
    expressions.push_back(emitThunk(item->getBody()));
    outputMapping.push_back(output++);
  }

  return Object{
      {"project",
       Object{{"common", Object{{"emit", Object{{"outputMapping",
                                                 std::move(outputMapping)}}}}},
              {"input", std::move(input)},
              {"expressions", std::move(expressions)}}}};
}

llvm::json::Value SubstraitEmitter::emitRel(const anf::Rel *rel) {
  switch (rel->getRelKind()) {
  case anf::RelKind::FromRel:
    return emitFromRel(anf::FromRel::cast(rel));
  case anf::RelKind::SelectRel:
    return emitSelectRel(anf::SelectRel::cast(rel));
  case anf::RelKind::WhereRel:
    return emitWhereRel(anf::WhereRel::cast(rel));
  case anf::RelKind::DistinctRel:
    return emitDistinctRel(anf::DistinctRel::cast(rel));
  case anf::RelKind::DropRel:
    return emitDropRel(anf::DropRel::cast(rel));
  case anf::RelKind::RenameRel:
    return emitRenameRel(anf::RenameRel::cast(rel));
  case anf::RelKind::ExtendRel:
    return emitExtendRel(anf::ExtendRel::cast(rel));
  }
  util::yuzu_unreachable("unhandled relation kind while emitting Substrait");
}

std::string SubstraitEmitter::emit(const anf::Root *root) {
  // Find the query: the relational expression at the program's tail. Any rel
  // kind can be the tail (e.g. a query ending in `where`).
  const anf::Rel *query = nullptr;
  for (const anf::Stmt *stmt : root->getStmts()) {
    if (const auto *exprStmt = anf::ExprStmt::cast(stmt)) {
      if (const auto *rel = anf::Rel::cast(exprStmt->getValue())) {
        query = rel;
      }
    }
  }
  if (query == nullptr) {
    return "";
  }

  // Build the relation first, so function registration populates the tables.
  Value relation = emitRel(query);

  // Output column names come from the query's row type, so aliased, bare-ident,
  // and generated (`%gN`) names all carry through.
  Array names;
  if (const types::StructType *outRow = rowStruct(query->getType())) {
    for (const types::StructField &field : outRow->getFields()) {
      names.push_back(util::toUtf8(field.name));
    }
  }

  Object plan{{"extensionUris", emitExtensionUris()},
              {"extensions", emitExtensions()},
              {"relations",
               Array{Object{{"root", Object{{"input", std::move(relation)},
                                            {"names", std::move(names)}}}}}}};

  std::string out;
  llvm::raw_string_ostream os(out);
  os << llvm::formatv("{0:2}", Value(std::move(plan)));
  return out;
}

} // namespace yuzu::substrait
