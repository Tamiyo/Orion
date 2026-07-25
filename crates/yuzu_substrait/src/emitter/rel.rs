use std::collections::HashSet;

use substrait::proto::{
    AggregateRel, Expression, FilterRel, FunctionArgument, JoinRel, NamedStruct, ProjectRel,
    ReadRel, Rel, RelCommon,
    aggregate_rel::Grouping,
    expression::{RexType, ScalarFunction},
    function_argument::ArgType,
    join_rel::JoinType,
    read_rel::{NamedTable, ReadType},
    rel::RelType,
    rel_common::{Emit, EmitKind},
    r#type,
};
use yuzu_anf::anf::{Ident, JoinCondition, JoinKind, Rel as AnfRel, RelId, SelectItem, Thunk};
use yuzu_core::adt::SymbolId;
use yuzu_types::TypeId;

use crate::emitter::expr::selection;
use crate::emitter::extensions::{BOOLEAN_URN, COMPARISON_URN};
use crate::emitter::types::nullable;
use crate::emitter::{SubstraitEmitter, Unsupported};

impl SubstraitEmitter<'_> {
    pub(crate) fn emit_rel(&mut self, id: RelId) -> Result<Rel, Unsupported> {
        let anf = self.anf;
        let rel_type = match anf.rel(id) {
            AnfRel::From { relation, ty, .. } => self.emit_from(*relation, *ty),
            AnfRel::Join {
                left,
                right,
                kind,
                condition,
                ty,
            } => self.emit_join(*left, *right, *kind, condition, *ty)?,
            AnfRel::Select { input, items, .. } => self.emit_select(*input, items)?,
            AnfRel::Where {
                input, predicate, ..
            } => self.emit_where(*input, predicate)?,
            AnfRel::Distinct { input, ty } => self.emit_distinct(*input, *ty)?,
            AnfRel::Drop { input, columns, .. } => self.emit_drop(*input, columns)?,
            // Rename is positionally a no-op: the new names live in the output
            // row type and surface as the plan's output names.
            AnfRel::Rename { input, .. } => return self.emit_rel(*input),
            AnfRel::Extend { input, items, .. } => self.emit_extend(*input, items)?,
        };
        Ok(Rel {
            rel_type: Some(rel_type),
        })
    }

    fn emit_from(&mut self, relation: Ident, ty: TypeId) -> RelType {
        let fields = self.row_fields(ty);
        let names = fields
            .iter()
            .map(|&(name, _)| self.interner.text(name).to_string())
            .collect();
        let types = fields.iter().map(|&(_, ty)| self.emit_type(ty)).collect();
        RelType::Read(Box::new(ReadRel {
            base_schema: Some(NamedStruct {
                names,
                r#struct: Some(r#type::Struct {
                    types,
                    nullability: nullable(),
                    ..Default::default()
                }),
            }),
            read_type: Some(ReadType::NamedTable(NamedTable {
                names: vec![self.interner.text(relation.name).to_string()],
                ..Default::default()
            })),
            ..Default::default()
        }))
    }

    fn emit_join(
        &mut self,
        left: RelId,
        right: RelId,
        kind: JoinKind,
        condition: &JoinCondition,
        ty: TypeId,
    ) -> Result<RelType, Unsupported> {
        let left_ty = self.rel_ty(left);
        let right_ty = self.rel_ty(right);
        let left_width = self.row_fields(left_ty).len() as i32;

        let left_rel = self.emit_rel(left)?;
        let right_rel = self.emit_rel(right)?;

        let (expression, common) = match condition {
            JoinCondition::On(thunk) => {
                // An `on` join emits the concatenation, so its own row is what
                // the condition indexes.
                self.row = self.row_ty(ty);
                (self.emit_thunk(thunk)?, None)
            }
            JoinCondition::Using(columns) => (
                self.emit_using(left_ty, right_ty, left_width, columns),
                Self::emit_common(self.using_output_mapping(right_ty, left_width, columns)),
            ),
        };

        Ok(RelType::Join(Box::new(JoinRel {
            common,
            left: Some(Box::new(left_rel)),
            right: Some(Box::new(right_rel)),
            expression: Some(Box::new(expression)),
            r#type: join_type(kind) as i32,
            ..Default::default()
        })))
    }

    fn emit_using(
        &mut self,
        left_ty: TypeId,
        right_ty: TypeId,
        left_width: i32,
        columns: &[Ident],
    ) -> Expression {
        let mut condition: Option<Expression> = None;
        for column in columns {
            let equality = self.emit_using_equality(left_ty, right_ty, left_width, *column);
            condition = Some(match condition {
                Some(left) => self.emit_and(left, equality),
                None => equality,
            });
        }
        condition.expect("a `using` clause always has a column")
    }

    fn emit_using_equality(
        &mut self,
        left_ty: TypeId,
        right_ty: TypeId,
        left_width: i32,
        column: Ident,
    ) -> Expression {
        let left_index = self.field_index(self.row_ty(left_ty), column.name);
        let right_index = left_width + self.field_index(self.row_ty(right_ty), column.name);

        let code = self.type_code(self.field_ty(left_ty, column.name));
        let anchor = self
            .extensions
            .register(COMPARISON_URN, format!("equal:{code}_{code}"));
        self.emit_bool_function(anchor, vec![selection(left_index), selection(right_index)])
    }

    fn emit_and(&mut self, left: Expression, right: Expression) -> Expression {
        let anchor = self
            .extensions
            .register(BOOLEAN_URN, "and:bool_bool".to_string());
        self.emit_bool_function(anchor, vec![left, right])
    }

    fn emit_bool_function(&mut self, anchor: u32, arguments: Vec<Expression>) -> Expression {
        let bool_ty = self.emit_type(self.types.bool_ty());
        Expression {
            rex_type: Some(RexType::ScalarFunction(ScalarFunction {
                function_reference: anchor,
                output_type: Some(bool_ty),
                arguments: arguments
                    .into_iter()
                    .map(|value| FunctionArgument {
                        arg_type: Some(ArgType::Value(value)),
                    })
                    .collect(),
                ..Default::default()
            })),
        }
    }

    fn using_output_mapping(
        &self,
        right_ty: TypeId,
        left_width: i32,
        columns: &[Ident],
    ) -> Vec<i32> {
        let keys: HashSet<SymbolId> = columns.iter().map(|column| column.name).collect();
        (0..left_width)
            .chain(
                self.row_fields(right_ty)
                    .iter()
                    .enumerate()
                    .filter(|(_, (name, _))| !keys.contains(name))
                    .map(|(index, _)| left_width + index as i32),
            )
            .collect()
    }

    fn field_ty(&self, rel_ty: TypeId, name: SymbolId) -> TypeId {
        self.row_fields(rel_ty)
            .iter()
            .find(|(field, _)| *field == name)
            .map(|&(_, ty)| ty)
            .expect("a `using` column is in both rows")
    }

    fn emit_select(&mut self, input: RelId, items: &[SelectItem]) -> Result<RelType, Unsupported> {
        let input_ty = self.rel_ty(input);
        let input_columns = self.row_fields(input_ty).len() as i32;
        let input = self.emit_rel(input)?;
        self.row = self.row_ty(input_ty);
        let expressions = self.emit_select_items(items)?;
        let output_mapping = (input_columns..input_columns + expressions.len() as i32).collect();
        Ok(RelType::Project(Box::new(ProjectRel {
            common: Self::emit_common(output_mapping),
            input: Some(Box::new(input)),
            expressions,
            ..Default::default()
        })))
    }

    fn emit_where(&mut self, input: RelId, predicate: &Thunk) -> Result<RelType, Unsupported> {
        let input_ty = self.rel_ty(input);
        let input = self.emit_rel(input)?;
        self.row = self.row_ty(input_ty);
        let condition = self.emit_thunk(predicate)?;
        Ok(RelType::Filter(Box::new(FilterRel {
            input: Some(Box::new(input)),
            condition: Some(Box::new(condition)),
            ..Default::default()
        })))
    }

    fn emit_distinct(&mut self, input: RelId, ty: TypeId) -> Result<RelType, Unsupported> {
        let columns = self.row_fields(ty).len() as i32;
        let input = self.emit_rel(input)?;
        Ok(RelType::Aggregate(Box::new(AggregateRel {
            input: Some(Box::new(input)),
            grouping_expressions: (0..columns).map(selection).collect(),
            groupings: vec![Grouping {
                expression_references: (0..columns as u32).collect(),
                ..Default::default()
            }],
            measures: Vec::new(),
            ..Default::default()
        })))
    }

    fn emit_drop(&mut self, input: RelId, columns: &[Ident]) -> Result<RelType, Unsupported> {
        let output_mapping = self
            .row_fields(self.rel_ty(input))
            .iter()
            .enumerate()
            .filter(|(_, (field, _))| !columns.iter().any(|column| column.name == *field))
            .map(|(index, _)| index as i32)
            .collect();
        let input = self.emit_rel(input)?;
        Ok(RelType::Project(Box::new(ProjectRel {
            common: Self::emit_common(output_mapping),
            input: Some(Box::new(input)),
            expressions: Vec::new(),
            ..Default::default()
        })))
    }

    fn emit_extend(&mut self, input: RelId, items: &[SelectItem]) -> Result<RelType, Unsupported> {
        let input_ty = self.rel_ty(input);
        let input_columns = self.row_fields(input_ty).len() as i32;
        let input = self.emit_rel(input)?;
        self.row = self.row_ty(input_ty);
        let expressions = self.emit_select_items(items)?;
        let output_mapping = (0..input_columns)
            .chain(input_columns..input_columns + expressions.len() as i32)
            .collect();
        Ok(RelType::Project(Box::new(ProjectRel {
            common: Self::emit_common(output_mapping),
            input: Some(Box::new(input)),
            expressions,
            ..Default::default()
        })))
    }

    fn emit_common(output_mapping: Vec<i32>) -> Option<RelCommon> {
        Some(RelCommon {
            emit_kind: Some(EmitKind::Emit(Emit { output_mapping })),
            ..Default::default()
        })
    }

    fn emit_select_items(&mut self, items: &[SelectItem]) -> Result<Vec<Expression>, Unsupported> {
        items
            .iter()
            .map(|item| self.emit_thunk(&item.body))
            .collect()
    }

    pub(crate) fn rel_ty(&self, id: RelId) -> TypeId {
        match self.anf.rel(id) {
            AnfRel::From { ty, .. }
            | AnfRel::Join { ty, .. }
            | AnfRel::Select { ty, .. }
            | AnfRel::Where { ty, .. }
            | AnfRel::Distinct { ty, .. }
            | AnfRel::Drop { ty, .. }
            | AnfRel::Rename { ty, .. }
            | AnfRel::Extend { ty, .. } => *ty,
        }
    }
}

fn join_type(kind: JoinKind) -> JoinType {
    match kind {
        JoinKind::Inner => JoinType::Inner,
        JoinKind::Left => JoinType::Left,
        JoinKind::Right => JoinType::Right,
        JoinKind::Full => JoinType::Outer,
    }
}

#[cfg(test)]
mod tests {
    use expect_test::expect;

    use crate::emitter::test_support::{TABLE, check};

    #[test]
    fn emits_read_and_project() {
        check(
            &format!("{TABLE}from t |> select a, a + b as s"),
            expect![[r#"
            {
              "version": {
                "minorNumber": 85,
                "producer": "yuzu"
              },
              "extensionUrns": [
                {
                  "extensionUrnAnchor": 1,
                  "urn": "extension:io.substrait:functions_arithmetic"
                }
              ],
              "extensions": [
                {
                  "extensionFunction": {
                    "extensionUrnReference": 1,
                    "functionAnchor": 1,
                    "name": "add:i32_i32"
                  }
                }
              ],
              "relations": [
                {
                  "root": {
                    "input": {
                      "project": {
                        "common": {
                          "emit": {
                            "outputMapping": [
                              2,
                              3
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
                            "selection": {
                              "directReference": {
                                "structField": {}
                              },
                              "rootReference": {}
                            }
                          },
                          {
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
                                    "selection": {
                                      "directReference": {
                                        "structField": {
                                          "field": 1
                                        }
                                      },
                                      "rootReference": {}
                                    }
                                  }
                                }
                              ],
                              "outputType": {
                                "i32": {
                                  "nullability": "NULLABILITY_NULLABLE"
                                }
                              }
                            }
                          }
                        ]
                      }
                    },
                    "names": [
                      "a",
                      "s"
                    ]
                  }
                }
              ]
            }"#]],
        );
    }

    const JOIN_TABLES: &str = "struct Other { a: int32, c: int32 }\ntable u = Other\nstruct Codes { c: int32, d: int32 }\ntable v = Codes\n";

    #[test]
    fn emits_inner_join_on_condition() {
        check(
            &format!("{TABLE}{JOIN_TABLES}from t e |> join v x on e.b == x.c"),
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
                        "name": "equal:i32_i32"
                      }
                    }
                  ],
                  "relations": [
                    {
                      "root": {
                        "input": {
                          "join": {
                            "left": {
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
                            "right": {
                              "read": {
                                "baseSchema": {
                                  "names": [
                                    "c",
                                    "d"
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
                                    "v"
                                  ]
                                }
                              }
                            },
                            "expression": {
                              "scalarFunction": {
                                "functionReference": 1,
                                "arguments": [
                                  {
                                    "value": {
                                      "selection": {
                                        "directReference": {
                                          "structField": {
                                            "field": 1
                                          }
                                        },
                                        "rootReference": {}
                                      }
                                    }
                                  },
                                  {
                                    "value": {
                                      "selection": {
                                        "directReference": {
                                          "structField": {
                                            "field": 2
                                          }
                                        },
                                        "rootReference": {}
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
                            },
                            "type": "JOIN_TYPE_INNER"
                          }
                        },
                        "names": [
                          "a",
                          "b",
                          "c",
                          "d"
                        ]
                      }
                    }
                  ]
                }"#]],
        );
    }

    #[test]
    fn emits_left_join_using_one_key() {
        check(
            &format!("{TABLE}{JOIN_TABLES}from t |> left join u using (a)"),
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
                        "name": "equal:i32_i32"
                      }
                    }
                  ],
                  "relations": [
                    {
                      "root": {
                        "input": {
                          "join": {
                            "common": {
                              "emit": {
                                "outputMapping": [
                                  0,
                                  1,
                                  3
                                ]
                              }
                            },
                            "left": {
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
                            "right": {
                              "read": {
                                "baseSchema": {
                                  "names": [
                                    "a",
                                    "c"
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
                                    "u"
                                  ]
                                }
                              }
                            },
                            "expression": {
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
                                      "selection": {
                                        "directReference": {
                                          "structField": {
                                            "field": 2
                                          }
                                        },
                                        "rootReference": {}
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
                            },
                            "type": "JOIN_TYPE_LEFT"
                          }
                        },
                        "names": [
                          "a",
                          "b",
                          "c"
                        ]
                      }
                    }
                  ]
                }"#]],
        );
    }

    #[test]
    fn places_a_using_joins_right_columns_after_its_dropped_keys() {
        check(
            &format!("{TABLE}{JOIN_TABLES}from t |> join u d using (a) |> select d.c"),
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
                        "name": "equal:i32_i32"
                      }
                    }
                  ],
                  "relations": [
                    {
                      "root": {
                        "input": {
                          "project": {
                            "common": {
                              "emit": {
                                "outputMapping": [
                                  3
                                ]
                              }
                            },
                            "input": {
                              "join": {
                                "common": {
                                  "emit": {
                                    "outputMapping": [
                                      0,
                                      1,
                                      3
                                    ]
                                  }
                                },
                                "left": {
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
                                "right": {
                                  "read": {
                                    "baseSchema": {
                                      "names": [
                                        "a",
                                        "c"
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
                                        "u"
                                      ]
                                    }
                                  }
                                },
                                "expression": {
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
                                          "selection": {
                                            "directReference": {
                                              "structField": {
                                                "field": 2
                                              }
                                            },
                                            "rootReference": {}
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
                                },
                                "type": "JOIN_TYPE_INNER"
                              }
                            },
                            "expressions": [
                              {
                                "selection": {
                                  "directReference": {
                                    "structField": {
                                      "field": 2
                                    }
                                  },
                                  "rootReference": {}
                                }
                              }
                            ]
                          }
                        },
                        "names": [
                          "c"
                        ]
                      }
                    }
                  ]
                }"#]],
        );
    }

    #[test]
    fn resolves_the_right_sides_using_key_to_the_emitted_column() {
        check(
            &format!("{TABLE}{JOIN_TABLES}from t |> join u d using (a) |> select d.a"),
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
                        "name": "equal:i32_i32"
                      }
                    }
                  ],
                  "relations": [
                    {
                      "root": {
                        "input": {
                          "project": {
                            "common": {
                              "emit": {
                                "outputMapping": [
                                  3
                                ]
                              }
                            },
                            "input": {
                              "join": {
                                "common": {
                                  "emit": {
                                    "outputMapping": [
                                      0,
                                      1,
                                      3
                                    ]
                                  }
                                },
                                "left": {
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
                                "right": {
                                  "read": {
                                    "baseSchema": {
                                      "names": [
                                        "a",
                                        "c"
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
                                        "u"
                                      ]
                                    }
                                  }
                                },
                                "expression": {
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
                                          "selection": {
                                            "directReference": {
                                              "structField": {
                                                "field": 2
                                              }
                                            },
                                            "rootReference": {}
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
                                },
                                "type": "JOIN_TYPE_INNER"
                              }
                            },
                            "expressions": [
                              {
                                "selection": {
                                  "directReference": {
                                    "structField": {}
                                  },
                                  "rootReference": {}
                                }
                              }
                            ]
                          }
                        },
                        "names": [
                          "a"
                        ]
                      }
                    }
                  ]
                }"#]],
        );
    }

    #[test]
    fn emits_right_join_on_condition() {
        check(
            &format!("{TABLE}{JOIN_TABLES}from t |> right join v on b == c"),
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
                        "name": "equal:i32_i32"
                      }
                    }
                  ],
                  "relations": [
                    {
                      "root": {
                        "input": {
                          "join": {
                            "left": {
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
                            "right": {
                              "read": {
                                "baseSchema": {
                                  "names": [
                                    "c",
                                    "d"
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
                                    "v"
                                  ]
                                }
                              }
                            },
                            "expression": {
                              "scalarFunction": {
                                "functionReference": 1,
                                "arguments": [
                                  {
                                    "value": {
                                      "selection": {
                                        "directReference": {
                                          "structField": {
                                            "field": 1
                                          }
                                        },
                                        "rootReference": {}
                                      }
                                    }
                                  },
                                  {
                                    "value": {
                                      "selection": {
                                        "directReference": {
                                          "structField": {
                                            "field": 2
                                          }
                                        },
                                        "rootReference": {}
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
                            },
                            "type": "JOIN_TYPE_RIGHT"
                          }
                        },
                        "names": [
                          "a",
                          "b",
                          "c",
                          "d"
                        ]
                      }
                    }
                  ]
                }"#]],
        );
    }

    #[test]
    fn emits_full_join_over_a_joined_row() {
        check(
            &format!("{TABLE}{JOIN_TABLES}from t |> full join v on b == c |> select a, d"),
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
                        "name": "equal:i32_i32"
                      }
                    }
                  ],
                  "relations": [
                    {
                      "root": {
                        "input": {
                          "project": {
                            "common": {
                              "emit": {
                                "outputMapping": [
                                  4,
                                  5
                                ]
                              }
                            },
                            "input": {
                              "join": {
                                "left": {
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
                                "right": {
                                  "read": {
                                    "baseSchema": {
                                      "names": [
                                        "c",
                                        "d"
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
                                        "v"
                                      ]
                                    }
                                  }
                                },
                                "expression": {
                                  "scalarFunction": {
                                    "functionReference": 1,
                                    "arguments": [
                                      {
                                        "value": {
                                          "selection": {
                                            "directReference": {
                                              "structField": {
                                                "field": 1
                                              }
                                            },
                                            "rootReference": {}
                                          }
                                        }
                                      },
                                      {
                                        "value": {
                                          "selection": {
                                            "directReference": {
                                              "structField": {
                                                "field": 2
                                              }
                                            },
                                            "rootReference": {}
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
                                },
                                "type": "JOIN_TYPE_OUTER"
                              }
                            },
                            "expressions": [
                              {
                                "selection": {
                                  "directReference": {
                                    "structField": {}
                                  },
                                  "rootReference": {}
                                }
                              },
                              {
                                "selection": {
                                  "directReference": {
                                    "structField": {
                                      "field": 3
                                    }
                                  },
                                  "rootReference": {}
                                }
                              }
                            ]
                          }
                        },
                        "names": [
                          "a",
                          "d"
                        ]
                      }
                    }
                  ]
                }"#]],
        );
    }

    #[test]
    fn emits_pipeline_stages() {
        check(
            &format!("{TABLE}from t |> extend a * 2 as d |> drop b |> rename a as x |> distinct"),
            expect![[r#"
                {
                  "version": {
                    "minorNumber": 85,
                    "producer": "yuzu"
                  },
                  "extensionUrns": [
                    {
                      "extensionUrnAnchor": 1,
                      "urn": "extension:io.substrait:functions_arithmetic"
                    }
                  ],
                  "extensions": [
                    {
                      "extensionFunction": {
                        "extensionUrnReference": 1,
                        "functionAnchor": 1,
                        "name": "multiply:i32_i32"
                      }
                    }
                  ],
                  "relations": [
                    {
                      "root": {
                        "input": {
                          "aggregate": {
                            "input": {
                              "project": {
                                "common": {
                                  "emit": {
                                    "outputMapping": [
                                      0,
                                      2
                                    ]
                                  }
                                },
                                "input": {
                                  "project": {
                                    "common": {
                                      "emit": {
                                        "outputMapping": [
                                          0,
                                          1,
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
                                                  "i32": 2
                                                }
                                              }
                                            }
                                          ],
                                          "outputType": {
                                            "i32": {
                                              "nullability": "NULLABILITY_NULLABLE"
                                            }
                                          }
                                        }
                                      }
                                    ]
                                  }
                                }
                              }
                            },
                            "groupings": [
                              {
                                "expressionReferences": [
                                  0,
                                  1
                                ]
                              }
                            ],
                            "groupingExpressions": [
                              {
                                "selection": {
                                  "directReference": {
                                    "structField": {}
                                  },
                                  "rootReference": {}
                                }
                              },
                              {
                                "selection": {
                                  "directReference": {
                                    "structField": {
                                      "field": 1
                                    }
                                  },
                                  "rootReference": {}
                                }
                              }
                            ]
                          }
                        },
                        "names": [
                          "x",
                          "d"
                        ]
                      }
                    }
                  ]
                }"#]],
        );
    }
}
