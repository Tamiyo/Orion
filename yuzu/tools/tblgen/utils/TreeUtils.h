#ifndef YUZU_TOOLS_TBLGEN_UTILS_TREE_UTILS_H
#define YUZU_TOOLS_TBLGEN_UTILS_TREE_UTILS_H

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>
#include <llvm/TableGen/Error.h>
#include <llvm/TableGen/Record.h>

#include <string>
#include <variant>
#include <vector>

namespace yuzu::tools {

//===----------------------------------------------------------------------===//
// Field mirrors
//===----------------------------------------------------------------------===//
//
// Variant/Node/Base/Def metadata can be pulled off `Record *` cheaply at the
// emit site, so we don't mirror those. Field subclasses get mirrored because
// the generator dispatches over them (`std::visit`), which gives exhaustive
// matching when a new Field subclass shows up.

/// Mirror of `class Native<string name> : Field`.
struct Native {
  std::string name;
  bool isTrivial;
};

/// Mirror of `class EnumCase<string name, string value>`. Not a Field
/// itself; appears inside `Enum.cases`.
struct EnumCase {
  std::string name;
  std::string value;
};

/// Mirror of `class Enum<string nativeType = ""> : Field`.
struct Enum {
  std::string type;
  std::vector<EnumCase> cases;
};

/// Mirror of `class Child<Def type> : Field`.
struct Child {
  std::string typeName;
};

/// Mirror of `class Val<Def type> : Field`.
struct Val {
  std::string typeName;
};

/// Tagged union over every `Field` subclass the codegen knows how to emit.
/// Adding a new `Field` subclass means: declare a struct, plumb it through
/// `FieldKind`, and extend `parseFieldKind`.
using FieldKind = std::variant<Native, Enum, Child, Val>;

/// A `Field` paired with the `$name` it was bound to inside a `Node`'s
/// `Fields` dag. The name (capitalized) becomes the accessor suffix.
struct NamedField {
  std::string name;
  FieldKind kind;
};

//===----------------------------------------------------------------------===//
// Parsers
//===----------------------------------------------------------------------===//

inline Native parseNative(const llvm::Record *record) {
  return Native{
      .name = record->getValueAsString("Name").str(),
      .isTrivial = record->getValueAsBit("IsTrivial"),
  };
}

inline EnumCase parseEnumCase(const llvm::Record *record) {
  return EnumCase{
      .name = record->getValueAsString("Name").str(),
      .value = record->getValueAsString("Value").str(),
  };
}

inline Enum parseEnum(const llvm::Record *record) {
  const std::vector<const llvm::Record *> caseRecords =
      record->getValueAsListOfDefs("Cases");
  std::vector<EnumCase> cases;
  cases.reserve(caseRecords.size());
  for (const llvm::Record *c : caseRecords) {
    cases.push_back(parseEnumCase(c));
  }
  return Enum{
      .type = record->getValueAsString("Type").str(),
      .cases = std::move(cases),
  };
}

inline Child parseChild(const llvm::Record *record) {
  return Child{
      .typeName = record->getValueAsDef("Type")->getName().str(),
  };
}

inline Val parseVal(const llvm::Record *record) {
  return Val{
      .typeName = record->getValueAsDef("Type")->getName().str(),
  };
}

/// Dispatch a generic `Field`-derived record to the matching parser.
inline FieldKind parseFieldKind(const llvm::Record *record) {
  if (record->isSubClassOf("Native"))
    return parseNative(record);
  if (record->isSubClassOf("Enum"))
    return parseEnum(record);
  if (record->isSubClassOf("Child"))
    return parseChild(record);
  if (record->isSubClassOf("Val"))
    return parseVal(record);
  llvm::PrintFatalError(record->getLoc(),
                        "yuzu-tblgen: unknown Field subclass: " +
                            record->getName().str());
}

/// Walk a `Node`'s `Fields` dag and turn each entry into a `NamedField`.
inline std::vector<NamedField> parseFields(const llvm::Record *nodeRecord) {
  const llvm::DagInit *dag = nodeRecord->getValueAsDag("Fields");
  std::vector<NamedField> fields;
  fields.reserve(dag->getNumArgs());
  for (unsigned i = 0; i < dag->getNumArgs(); ++i) {
    const llvm::Init *arg = dag->getArg(i);
    const auto *defInit = llvm::dyn_cast<llvm::DefInit>(arg);
    if (!defInit) {
      llvm::PrintFatalError(nodeRecord->getLoc(),
                            "yuzu-tblgen: Fields entry must be a Field def, "
                            "got: " +
                                arg->getAsString());
    }
    fields.push_back(NamedField{
        .name = dag->getArgNameStr(i).str(),
        .kind = parseFieldKind(defInit->getDef()),
    });
  }
  return fields;
}

} // namespace yuzu::tools

#endif // YUZU_TOOLS_TBLGEN_UTILS_TREE_UTILS_H