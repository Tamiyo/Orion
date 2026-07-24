use substrait::proto::extensions::{
    SimpleExtensionDeclaration, SimpleExtensionUrn,
    simple_extension_declaration::{ExtensionFunction, MappingType},
};
use yuzu_anf::anf::Op;

// Substrait standard extensions (the function families DuckDB consumes).
const ARITHMETIC_URN: &str = "extension:io.substrait:functions_arithmetic";
const COMPARISON_URN: &str = "extension:io.substrait:functions_comparison";
pub(crate) const BOOLEAN_URN: &str = "extension:io.substrait:functions_boolean";

/// Map an ANF builtin operator to its Substrait extension function. Membership
/// tests are handled separately (`SingularOrList`); an operator with no
/// Substrait equivalent yet (e.g. `**`, unary `+`) returns none.
pub(crate) fn function_target(op: Op) -> Option<(&'static str, &'static str)> {
    let target = match op {
        Op::Add => (ARITHMETIC_URN, "add"),
        Op::Sub => (ARITHMETIC_URN, "subtract"),
        Op::Mul => (ARITHMETIC_URN, "multiply"),
        Op::Div => (ARITHMETIC_URN, "divide"),
        Op::ShiftLeft => (ARITHMETIC_URN, "shift_left"),
        Op::ShiftRight => (ARITHMETIC_URN, "shift_right"),
        Op::UnaryNeg => (ARITHMETIC_URN, "negate"),
        Op::Eq => (COMPARISON_URN, "equal"),
        Op::Neq => (COMPARISON_URN, "not_equal"),
        Op::Lt => (COMPARISON_URN, "lt"),
        Op::Lte => (COMPARISON_URN, "lte"),
        Op::Gt => (COMPARISON_URN, "gt"),
        Op::Gte => (COMPARISON_URN, "gte"),
        Op::And => (BOOLEAN_URN, "and"),
        Op::Or => (BOOLEAN_URN, "or"),
        Op::UnaryNot => (BOOLEAN_URN, "not"),
        Op::Pow | Op::In | Op::NotIn | Op::UnaryPos => return None,
    };
    Some(target)
}

/// The plan's extension tables, built up as functions are registered.
#[derive(Default)]
pub(crate) struct Extensions {
    urns: Vec<&'static str>,
    functions: Vec<(u32, String)>,
}

impl Extensions {
    /// Intern a `(urn, name)` function; returns its function anchor.
    pub(crate) fn register(&mut self, urn: &'static str, name: String) -> u32 {
        let urn_anchor = match self.urns.iter().position(|&candidate| candidate == urn) {
            Some(index) => index as u32 + 1,
            None => {
                self.urns.push(urn);
                self.urns.len() as u32
            }
        };
        self.functions.push((urn_anchor, name));
        self.functions.len() as u32
    }

    pub(crate) fn urns(&self) -> Vec<SimpleExtensionUrn> {
        self.urns
            .iter()
            .enumerate()
            .map(|(index, &urn)| SimpleExtensionUrn {
                extension_urn_anchor: index as u32 + 1,
                urn: urn.to_string(),
            })
            .collect()
    }

    pub(crate) fn declarations(&self) -> Vec<SimpleExtensionDeclaration> {
        self.functions
            .iter()
            .enumerate()
            .map(|(index, (urn_anchor, name))| SimpleExtensionDeclaration {
                mapping_type: Some(MappingType::ExtensionFunction(ExtensionFunction {
                    extension_urn_reference: *urn_anchor,
                    function_anchor: index as u32 + 1,
                    name: name.clone(),
                })),
            })
            .collect()
    }
}
