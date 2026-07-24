use std::collections::HashMap;

use substrait::proto::{
    Expression, FunctionArgument,
    expression::{
        FieldReference, Literal, ReferenceSegment, RexType, ScalarFunction, SingularOrList,
        field_reference::{ReferenceType, RootReference, RootType},
        literal::{self, LiteralType},
        reference_segment,
    },
    function_argument::ArgType,
};
use yuzu_anf::AnfCtx;
use yuzu_anf::anf::{Atom, AtomId, BindingId, Const, Expr, ExprId, Op, Stmt, Thunk};
use yuzu_types::TypeId;

use crate::emitter::extensions::{BOOLEAN_URN, function_target};
use crate::emitter::{SubstraitEmitter, Unsupported};

struct ThunkDefs {
    defs: HashMap<BindingId, ExprId>,
}

impl ThunkDefs {
    fn new(anf: &AnfCtx, thunk: &Thunk) -> Self {
        let mut defs = HashMap::new();
        for &stmt in thunk.stmts.iter() {
            if let Stmt::Let { binding, expr } = anf.stmt(stmt) {
                defs.insert(*binding, *expr);
            }
        }
        Self { defs }
    }

    fn def(&self, binding: BindingId) -> Option<ExprId> {
        self.defs.get(&binding).copied()
    }
}

pub(crate) fn selection(index: i32) -> Expression {
    Expression {
        rex_type: Some(RexType::Selection(Box::new(FieldReference {
            reference_type: Some(ReferenceType::DirectReference(ReferenceSegment {
                reference_type: Some(reference_segment::ReferenceType::StructField(Box::new(
                    reference_segment::StructField {
                        field: index,
                        child: None,
                    },
                ))),
            })),
            root_type: Some(RootType::RootReference(RootReference {})),
        }))),
    }
}

fn literal(value: LiteralType) -> Expression {
    Expression {
        rex_type: Some(RexType::Literal(Literal {
            literal_type: Some(value),
            ..Default::default()
        })),
    }
}

impl SubstraitEmitter<'_> {
    pub(crate) fn emit_thunk(&mut self, thunk: &Thunk) -> Result<Expression, Unsupported> {
        let defs = ThunkDefs::new(self.anf, thunk);
        self.emit_atom(thunk.value, &defs)
    }

    fn emit_expr(&mut self, id: ExprId, defs: &ThunkDefs) -> Result<Expression, Unsupported> {
        let anf = self.anf;
        match anf.expr(id) {
            Expr::Atom { value } => self.emit_atom(*value, defs),
            Expr::Call { op, args, ty } => self.emit_call(id, *op, args, *ty, defs),
            Expr::ListInit { elements, .. } => Ok(self.emit_list_literal(elements)),
            Expr::FuncCall { callee, .. } => {
                let message = match *anf.atom(*callee) {
                    Atom::FuncRef { binding, .. } => format!(
                        "call to `{}` could not be fully reduced",
                        self.interner.text(anf.binding(binding).name.name)
                    ),
                    _ => "a call could not be fully reduced".to_string(),
                };
                Err(self.unsupported(id, message))
            }
            Expr::MethodCall { .. } => {
                Err(self.unsupported(id, "method calls are not yet supported in Substrait plans"))
            }
            Expr::StructInit { .. } => {
                Err(self.unsupported(id, "a struct value cannot be a query column"))
            }
            Expr::Rel(_) => Err(self.unsupported(id, "a query cannot be used as a column")),
        }
    }

    fn emit_atom(&mut self, id: AtomId, defs: &ThunkDefs) -> Result<Expression, Unsupported> {
        let expression = match *self.anf.atom(id) {
            Atom::Const(constant) => literal(self.literal_value(constant)),
            Atom::Var { binding } => match defs.def(binding) {
                Some(expr) => self.emit_expr(expr, defs)?,
                None => {
                    let name = self.interner.text(self.anf.binding(binding).name.name);
                    let message = if name.starts_with('%') {
                        "query column depends on a value that could not be fully reduced"
                            .to_string()
                    } else {
                        format!(
                            "query column depends on `{name}`, which could not be fully reduced"
                        )
                    };
                    return Err(self.unsupported_query(message));
                }
            },
            Atom::Field { base, field, .. } => {
                let row = match *self.anf.atom(base) {
                    Atom::Var { binding } => self.anf.binding(binding).ty,
                    _ => unreachable!("a field selection is rooted at the query row"),
                };
                selection(self.field_index(row, field.name))
            }
            Atom::FuncRef { .. } => {
                unreachable!("a function reference cannot be emitted as a column")
            }
        };
        Ok(expression)
    }

    fn emit_call(
        &mut self,
        id: ExprId,
        op: Op,
        args: &[AtomId],
        ty: TypeId,
        defs: &ThunkDefs,
    ) -> Result<Expression, Unsupported> {
        match op {
            Op::In | Op::NotIn => self.emit_membership(op, args, ty, defs),
            _ => self.emit_scalar_function(id, op, args, ty, defs),
        }
    }

    fn emit_scalar_function(
        &mut self,
        id: ExprId,
        op: Op,
        args: &[AtomId],
        ty: TypeId,
        defs: &ThunkDefs,
    ) -> Result<Expression, Unsupported> {
        let Some((urn, base)) = function_target(op) else {
            let message = format!("operator `{}` has no Substrait equivalent", op.symbol());
            return Err(self.unsupported(id, message));
        };

        let signature: Vec<&str> = args
            .iter()
            .map(|&arg| self.atom_type_code(arg, defs))
            .collect();
        let name = format!("{base}:{}", signature.join("_"));

        let mut arguments = Vec::new();
        for &arg in args {
            arguments.push(FunctionArgument {
                arg_type: Some(ArgType::Value(self.emit_atom(arg, defs)?)),
            });
        }

        let anchor = self.extensions.register(urn, name);
        Ok(Expression {
            rex_type: Some(RexType::ScalarFunction(ScalarFunction {
                function_reference: anchor,
                output_type: Some(self.emit_type(ty)),
                arguments,
                ..Default::default()
            })),
        })
    }

    fn emit_membership(
        &mut self,
        op: Op,
        args: &[AtomId],
        ty: TypeId,
        defs: &ThunkDefs,
    ) -> Result<Expression, Unsupported> {
        let [value, list] = args else {
            unreachable!("a membership test has a value and a list")
        };
        let value = self.emit_atom(*value, defs)?;
        let options = self.list_options(*list, defs)?;
        let test = Expression {
            rex_type: Some(RexType::SingularOrList(Box::new(SingularOrList {
                value: Some(Box::new(value)),
                options,
            }))),
        };
        Ok(match op {
            Op::In => test,
            _ => self.emit_not(test, ty),
        })
    }

    fn list_options(
        &mut self,
        list: AtomId,
        defs: &ThunkDefs,
    ) -> Result<Vec<Expression>, Unsupported> {
        let anf = self.anf;
        let expr = match *anf.atom(list) {
            Atom::Var { binding } => defs
                .def(binding)
                .expect("a membership list is always thunk-local"),
            _ => unreachable!("a membership list is a bound name"),
        };
        match anf.expr(expr) {
            Expr::ListInit { elements, .. } => elements
                .iter()
                .map(|&element| self.emit_atom(element, defs))
                .collect(),
            _ => unreachable!("a membership list is a list literal"),
        }
    }

    fn emit_not(&mut self, value: Expression, ty: TypeId) -> Expression {
        let anchor = self
            .extensions
            .register(BOOLEAN_URN, "not:bool".to_string());
        Expression {
            rex_type: Some(RexType::ScalarFunction(ScalarFunction {
                function_reference: anchor,
                output_type: Some(self.emit_type(ty)),
                arguments: vec![FunctionArgument {
                    arg_type: Some(ArgType::Value(value)),
                }],
                ..Default::default()
            })),
        }
    }

    fn emit_list_literal(&mut self, elements: &[AtomId]) -> Expression {
        let values = elements
            .iter()
            .map(|&element| match *self.anf.atom(element) {
                Atom::Const(constant) => Literal {
                    literal_type: Some(self.literal_value(constant)),
                    ..Default::default()
                },
                _ => unreachable!("non-constant list element while emitting Substrait"),
            })
            .collect();
        literal(LiteralType::List(literal::List { values }))
    }

    fn literal_value(&self, constant: Const) -> LiteralType {
        match constant {
            Const::Int { value } => match value.num_bits() {
                8 => LiteralType::I8(value.as_i64() as i32),
                16 => LiteralType::I16(value.as_i64() as i32),
                32 => LiteralType::I32(value.as_i64() as i32),
                _ => LiteralType::I64(value.as_i64()),
            },
            Const::Float { value } => match value.num_bits() {
                32 => LiteralType::Fp32(value.as_f64() as f32),
                _ => LiteralType::Fp64(value.as_f64()),
            },
            Const::Bool { value } => LiteralType::Boolean(value),
            Const::String { value } => LiteralType::String(self.interner.text(value).to_string()),
        }
    }

    fn atom_type_code(&self, id: AtomId, defs: &ThunkDefs) -> &'static str {
        match *self.anf.atom(id) {
            Atom::Const(Const::Int { value }) => match value.num_bits() {
                8 => "i8",
                16 => "i16",
                32 => "i32",
                _ => "i64",
            },
            Atom::Const(Const::Float { value }) => match value.num_bits() {
                32 => "fp32",
                _ => "fp64",
            },
            Atom::Const(Const::Bool { .. }) => "bool",
            Atom::Const(Const::String { .. }) => "string",
            Atom::Field { ty, .. } => self.type_code(ty),
            Atom::Var { binding } => {
                let expr = defs
                    .def(binding)
                    .expect("a signature atom is always thunk-local");
                self.expr_type_code(expr, defs)
            }
            Atom::FuncRef { .. } => {
                unreachable!("a function reference has no Substrait type code")
            }
        }
    }

    fn expr_type_code(&self, id: ExprId, defs: &ThunkDefs) -> &'static str {
        match self.anf.expr(id) {
            Expr::Atom { value } => self.atom_type_code(*value, defs),
            Expr::Call { ty, .. }
            | Expr::FuncCall { ty, .. }
            | Expr::MethodCall { ty, .. }
            | Expr::StructInit { ty, .. }
            | Expr::ListInit { ty, .. } => self.type_code(*ty),
            Expr::Rel(_) => unreachable!("a relation has no Substrait type code"),
        }
    }
}

#[cfg(test)]
mod tests {
    use expect_test::expect;

    use crate::emitter::test_support::{TABLE, check, check_error};

    #[test]
    fn reports_a_call_that_could_not_be_reduced() {
        check_error(
            &format!(
                "{TABLE}fn fact(n: int64) -> int64 {{ return n * fact(n - 1) }}\nfrom t |> select fact(3) as v"
            ),
            expect!["call to `fact` could not be fully reduced"],
        );
    }

    #[test]
    fn emits_literal_types() {
        check(
            &format!(
                "{TABLE}let big = 5000000000\nlet f = 1.5\nlet flag = true\nlet name = \"jon\"\nfrom t |> select big, f, flag, name"
            ),
            expect![[r#"
                {
                  "version": {
                    "minorNumber": 85,
                    "producer": "yuzu"
                  },
                  "relations": [
                    {
                      "root": {
                        "input": {
                          "project": {
                            "common": {
                              "emit": {
                                "outputMapping": [
                                  2,
                                  3,
                                  4,
                                  5
                                ]
                              }
                            },
                            "input": {
                              "read": {
                                "baseSchema": {
                                  "names": [
                                    "a",
                                    "b"
                                  ],
                                  "struct": {
                                    "types": [
                                      {
                                        "i32": {
                                          "nullability": "NULLABILITY_NULLABLE"
                                        }
                                      },
                                      {
                                        "i32": {
                                          "nullability": "NULLABILITY_NULLABLE"
                                        }
                                      }
                                    ],
                                    "nullability": "NULLABILITY_NULLABLE"
                                  }
                                },
                                "namedTable": {
                                  "names": [
                                    "t"
                                  ]
                                }
                              }
                            },
                            "expressions": [
                              {
                                "literal": {
                                  "i64": "5000000000"
                                }
                              },
                              {
                                "literal": {
                                  "fp64": 1.5
                                }
                              },
                              {
                                "literal": {
                                  "boolean": true
                                }
                              },
                              {
                                "literal": {
                                  "string": "jon"
                                }
                              }
                            ]
                          }
                        },
                        "names": [
                          "big",
                          "f",
                          "flag",
                          "name"
                        ]
                      }
                    }
                  ]
                }"#]],
        );
    }

    #[test]
    fn emits_list_literal_column() {
        check(
            &format!("{TABLE}let xs: List[int32] = [1, 2]\nfrom t |> select xs as l"),
            expect![[r#"
                {
                  "version": {
                    "minorNumber": 85,
                    "producer": "yuzu"
                  },
                  "relations": [
                    {
                      "root": {
                        "input": {
                          "project": {
                            "common": {
                              "emit": {
                                "outputMapping": [
                                  2
                                ]
                              }
                            },
                            "input": {
                              "read": {
                                "baseSchema": {
                                  "names": [
                                    "a",
                                    "b"
                                  ],
                                  "struct": {
                                    "types": [
                                      {
                                        "i32": {
                                          "nullability": "NULLABILITY_NULLABLE"
                                        }
                                      },
                                      {
                                        "i32": {
                                          "nullability": "NULLABILITY_NULLABLE"
                                        }
                                      }
                                    ],
                                    "nullability": "NULLABILITY_NULLABLE"
                                  }
                                },
                                "namedTable": {
                                  "names": [
                                    "t"
                                  ]
                                }
                              }
                            },
                            "expressions": [
                              {
                                "literal": {
                                  "list": {
                                    "values": [
                                      {
                                        "i32": 1
                                      },
                                      {
                                        "i32": 2
                                      }
                                    ]
                                  }
                                }
                              }
                            ]
                          }
                        },
                        "names": [
                          "l"
                        ]
                      }
                    }
                  ]
                }"#]],
        );
    }

    #[test]
    fn reports_an_unsupported_operator() {
        check_error(
            &format!("{TABLE}from t |> select a ** 2 as p"),
            expect!["operator `**` has no Substrait equivalent"],
        );
    }

    #[test]
    fn reports_a_struct_column() {
        check_error(
            &format!(
                "{TABLE}struct P {{ v: int64 }}\nlet p = P {{ v: 1 }}\nfrom t |> select p as q"
            ),
            expect!["a struct value cannot be a query column"],
        );
    }

    #[test]
    fn reports_an_unreduced_column_dependency() {
        check_error(
            &format!(
                "{TABLE}fn fact(n: int64) -> int64 {{ return n * fact(n - 1) }}\nlet v = fact(3)\nfrom t |> select v as w"
            ),
            expect!["query column depends on a value that could not be fully reduced"],
        );
    }

    #[test]
    fn emits_filter_with_literal() {
        check(
            &format!("{TABLE}from t |> where a > 1"),
            expect![[r#"
                {
                  "version": {
                    "minorNumber": 85,
                    "producer": "yuzu"
                  },
                  "extensionUrns": [
                    {
                      "extensionUrnAnchor": 1,
                      "urn": "extension:io.substrait:functions_comparison"
                    }
                  ],
                  "extensions": [
                    {
                      "extensionFunction": {
                        "extensionUrnReference": 1,
                        "functionAnchor": 1,
                        "name": "gt:i32_i32"
                      }
                    }
                  ],
                  "relations": [
                    {
                      "root": {
                        "input": {
                          "filter": {
                            "input": {
                              "read": {
                                "baseSchema": {
                                  "names": [
                                    "a",
                                    "b"
                                  ],
                                  "struct": {
                                    "types": [
                                      {
                                        "i32": {
                                          "nullability": "NULLABILITY_NULLABLE"
                                        }
                                      },
                                      {
                                        "i32": {
                                          "nullability": "NULLABILITY_NULLABLE"
                                        }
                                      }
                                    ],
                                    "nullability": "NULLABILITY_NULLABLE"
                                  }
                                },
                                "namedTable": {
                                  "names": [
                                    "t"
                                  ]
                                }
                              }
                            },
                            "condition": {
                              "scalarFunction": {
                                "functionReference": 1,
                                "arguments": [
                                  {
                                    "value": {
                                      "selection": {
                                        "directReference": {
                                          "structField": {}
                                        },
                                        "rootReference": {}
                                      }
                                    }
                                  },
                                  {
                                    "value": {
                                      "literal": {
                                        "i32": 1
                                      }
                                    }
                                  }
                                ],
                                "outputType": {
                                  "bool": {
                                    "nullability": "NULLABILITY_NULLABLE"
                                  }
                                }
                              }
                            }
                          }
                        },
                        "names": [
                          "a",
                          "b"
                        ]
                      }
                    }
                  ]
                }"#]],
        );
    }

    #[test]
    fn emits_membership_as_singular_or_list() {
        check(
            &format!("{TABLE}let xs: List[int32] = [1, 2]\nfrom t |> where a in xs"),
            expect![[r#"
                {
                  "version": {
                    "minorNumber": 85,
                    "producer": "yuzu"
                  },
                  "relations": [
                    {
                      "root": {
                        "input": {
                          "filter": {
                            "input": {
                              "read": {
                                "baseSchema": {
                                  "names": [
                                    "a",
                                    "b"
                                  ],
                                  "struct": {
                                    "types": [
                                      {
                                        "i32": {
                                          "nullability": "NULLABILITY_NULLABLE"
                                        }
                                      },
                                      {
                                        "i32": {
                                          "nullability": "NULLABILITY_NULLABLE"
                                        }
                                      }
                                    ],
                                    "nullability": "NULLABILITY_NULLABLE"
                                  }
                                },
                                "namedTable": {
                                  "names": [
                                    "t"
                                  ]
                                }
                              }
                            },
                            "condition": {
                              "singularOrList": {
                                "value": {
                                  "selection": {
                                    "directReference": {
                                      "structField": {}
                                    },
                                    "rootReference": {}
                                  }
                                },
                                "options": [
                                  {
                                    "literal": {
                                      "i32": 1
                                    }
                                  },
                                  {
                                    "literal": {
                                      "i32": 2
                                    }
                                  }
                                ]
                              }
                            }
                          }
                        },
                        "names": [
                          "a",
                          "b"
                        ]
                      }
                    }
                  ]
                }"#]],
        );
    }

    #[test]
    fn emits_not_in_wrapped_in_not() {
        check(
            &format!("{TABLE}let xs: List[int32] = [1, 2]\nfrom t |> where a not in xs"),
            expect![[r#"
                {
                  "version": {
                    "minorNumber": 85,
                    "producer": "yuzu"
                  },
                  "extensionUrns": [
                    {
                      "extensionUrnAnchor": 1,
                      "urn": "extension:io.substrait:functions_boolean"
                    }
                  ],
                  "extensions": [
                    {
                      "extensionFunction": {
                        "extensionUrnReference": 1,
                        "functionAnchor": 1,
                        "name": "not:bool"
                      }
                    }
                  ],
                  "relations": [
                    {
                      "root": {
                        "input": {
                          "filter": {
                            "input": {
                              "read": {
                                "baseSchema": {
                                  "names": [
                                    "a",
                                    "b"
                                  ],
                                  "struct": {
                                    "types": [
                                      {
                                        "i32": {
                                          "nullability": "NULLABILITY_NULLABLE"
                                        }
                                      },
                                      {
                                        "i32": {
                                          "nullability": "NULLABILITY_NULLABLE"
                                        }
                                      }
                                    ],
                                    "nullability": "NULLABILITY_NULLABLE"
                                  }
                                },
                                "namedTable": {
                                  "names": [
                                    "t"
                                  ]
                                }
                              }
                            },
                            "condition": {
                              "scalarFunction": {
                                "functionReference": 1,
                                "arguments": [
                                  {
                                    "value": {
                                      "singularOrList": {
                                        "value": {
                                          "selection": {
                                            "directReference": {
                                              "structField": {}
                                            },
                                            "rootReference": {}
                                          }
                                        },
                                        "options": [
                                          {
                                            "literal": {
                                              "i32": 1
                                            }
                                          },
                                          {
                                            "literal": {
                                              "i32": 2
                                            }
                                          }
                                        ]
                                      }
                                    }
                                  }
                                ],
                                "outputType": {
                                  "bool": {
                                    "nullability": "NULLABILITY_NULLABLE"
                                  }
                                }
                              }
                            }
                          }
                        },
                        "names": [
                          "a",
                          "b"
                        ]
                      }
                    }
                  ]
                }"#]],
        );
    }
}
