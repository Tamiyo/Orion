//===----------------------------------------------------------------------===//
// Tree schema.
//===----------------------------------------------------------------------===//
//
// Shared TableGen vocabulary for tree-shaped IRs (AST, HIR, MIR, ...). A
// tree consists of `Variant`s (discriminated unions over concrete `Node`s),
// each `Node` carrying a typed list of `Field`s. The `Field` hierarchy
// covers child references, native C++ types, and enum-shaped values.
//
//===----------------------------------------------------------------------===//

/// Empty dag operator used as the default for a `Node`'s `Fields`. Borrows
/// MLIR's `(ins ...)` convention so field lists read declaratively.
def ins;

/// Common base for every TableGen def consumed by yuzu-tblgen. `Summary` is
/// an optional one-line docstring forwarded into the emitted C++ as a
/// doxygen `///` comment.
class Def {
    string Summary = "";
}

/// The "root" of a grammar. For a grammar named e.g. `AstNode`, every node
/// in that grammar transitively reaches an `AstNode : Base` def via the
/// `Parent` chain. `Namespace` names the C++ namespace the codegen wraps
/// emitted types in.
class Base : Def {
    string Namespace = "";
    dag Fields = (ins);
}

/// An abstract node realized as a discriminated union of concrete `Node`s.
/// Every `Node` whose `Parent` points at this `Variant` becomes one of its
/// alternatives.
class Variant<Base parent> : Base {
    Def Parent = parent;
    dag Fields = (ins);
}

/// A concrete tree node with a fixed shape described by `Fields`. Each entry
/// in the dag becomes a typed accessor on the generated C++ class.
class Node<Base parent> : Def {
    Def Parent = parent;
    dag Fields = (ins);
}

//===----------------------------------------------------------------------===//
// Type schema.
//===----------------------------------------------------------------------===//

/// Base class for a typed slot inside a `Node`'s `Fields` dag.
class Field : Def {}

/// A reference to a primitive C++ type given by its source-level name. Set
/// `IsTrivial` when the type can be passed/returned by value cheaply.
class Native<string name> : Field {
    string Name = name;
    bit IsTrivial = 0;
}

/// A single case inside an `Enum`: the C++ enumerator `name` plus the
/// literal `value` it maps to.
class EnumCase<string name, string value = ""> {
    string Name = name;
    string Value = value;
}

/// A C++ enum, optionally backed by `nativeType`. `Cases` enumerates the
/// alternatives.
class Enum<string nativeType = ""> : Field {
    string Type = nativeType;
    list<EnumCase> Cases = [];
}

//===----------------------------------------------------------------------===//
// Field schema.
//===----------------------------------------------------------------------===//

/// A child of a tree, typically another `Node` or `Variant`.
class Child<Def type> : Field {
    Def Type = type;
}

/// A list children of a tree, typically another `Node` or `Variant`.
class Children<Def type> : Field {
    Def Type = type;
}

/// A child "value" of a tree — typically a concrete type like an enum or
/// primitive, as opposed to a structural child node.
class Val<Def type> : Field {
    Def Type = type;
}

/// A child list of "values" of a tree — typically a concrete type like an enum or
/// primitive, as opposed to a structural child node.
class Values<Def type> : Field {
    Def Type = type;
}

/// A custom field of a tree, requring manual implementation.
class Custom<Def type> : Field {
    Def Type = type;
}
