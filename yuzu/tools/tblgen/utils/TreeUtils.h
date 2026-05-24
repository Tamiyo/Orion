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

/// Mirror of `class Native<string name> : Field`. `viewName` is the
/// optional non-owning companion (e.g. `std::u32string_view`); empty
/// means "no separate view, use `name` directly."
struct Native {
  std::string name;
  std::string viewName;
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

/// Mirror of `class Children<Def type> : Field`. List form of Child.
struct Children {
  std::string typeName;
};

/// Mirror of `class Val<Def type> : Field`. `isTrivial` is lifted from
/// the inner type (a `Native`'s `IsTrivial` bit, or `true` for `Enum`)
/// so generators can pick `T` vs `const T &` at the boundary.
/// `accessorTypeName` is what should appear in an accessor signature:
/// `{typeName}View` when the inner Native opts into a view companion,
/// otherwise the same as `typeName`.
struct Val {
  std::string typeName;
  std::string accessorTypeName;
  bool isTrivial;
};

/// Mirror of `class Values<Def type> : Field`. List form of Val.
struct Values {
  std::string typeName;
};

/// Mirror of `class Custom<Def type> : Field`. Same conventions as
/// [[Val]] for `isTrivial` and `accessorTypeName`.
struct Custom {
  std::string typeName;
  std::string accessorTypeName;
  bool isTrivial;
};

/// Tagged union over every `Field` subclass the codegen knows how to emit.
/// Adding a new `Field` subclass means: declare a struct, plumb it through
/// `FieldKind`, and extend `parseFieldKind`.
using FieldKind =
    std::variant<Native, Enum, Child, Children, Val, Values, Custom>;

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
      .viewName = record->getValueAsString("ViewName").str(),
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

inline Children parseChildren(const llvm::Record *record) {
  return Children{
      .typeName = record->getValueAsDef("Type")->getName().str(),
  };
}

/// True when `typeDef` can be passed/returned by value cheaply: a
/// `Native` flagged `IsTrivial`, or any `Enum` (a small integer). Everything
/// else (e.g. `Native<"std::u32string">`) is treated as non-trivial so
/// generators wrap accessors/params in `const T &`.
inline bool isTrivialType(const llvm::Record *typeDef) {
  if (typeDef->isSubClassOf("Native")) {
    return typeDef->getValueAsBit("IsTrivial");
  }
  if (typeDef->isSubClassOf("Enum")) {
    return true;
  }
  return false;
}

/// The alias to use in an accessor signature for a `Custom<T>` / `Val<T>`.
/// `{T}View` when `T` is a Native with a non-empty `ViewName`; otherwise
/// the def-level type name. The `{T}View` alias itself is emitted by
/// [[emitNatives]] so signatures can name it.
inline std::string accessorTypeName(const llvm::Record *typeDef) {
  const std::string name = typeDef->getName().str();
  if (typeDef->isSubClassOf("Native") &&
      !typeDef->getValueAsString("ViewName").empty()) {
    return name + "View";
  }
  return name;
}

inline Val parseVal(const llvm::Record *record) {
  const llvm::Record *type = record->getValueAsDef("Type");
  return Val{
      .typeName = type->getName().str(),
      .accessorTypeName = accessorTypeName(type),
      .isTrivial = isTrivialType(type),
  };
}

inline Values parseValues(const llvm::Record *record) {
  return Values{
      .typeName = record->getValueAsDef("Type")->getName().str(),
  };
}

inline Custom parseCustom(const llvm::Record *record) {
  const llvm::Record *type = record->getValueAsDef("Type");
  return Custom{
      .typeName = type->getName().str(),
      .accessorTypeName = accessorTypeName(type),
      .isTrivial = isTrivialType(type),
  };
}

/// Dispatch a generic `Field`-derived record to the matching parser.
inline FieldKind parseFieldKind(const llvm::Record *record) {
  if (record->isSubClassOf("Native")) {
    return parseNative(record);
  }

  if (record->isSubClassOf("Enum")) {
    return parseEnum(record);
  }

  // Check the list forms before the singular ones: in TreeBase.td,
  // `Children` and `Values` are siblings of `Child`/`Val` rather than
  // subclasses, so the order is technically immaterial. Listed this way
  // for symmetry with the variant declaration above.
  if (record->isSubClassOf("Children")) {
    return parseChildren(record);
  }

  if (record->isSubClassOf("Child")) {
    return parseChild(record);
  }

  if (record->isSubClassOf("Values")) {
    return parseValues(record);
  }

  if (record->isSubClassOf("Val")) {
    return parseVal(record);
  }

  if (record->isSubClassOf("Custom")) {
    return parseCustom(record);
  }

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
