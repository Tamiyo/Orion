use substrait::proto::{
    AggregateRel, Expression, FilterRel, NamedStruct, ProjectRel, ReadRel, Rel, RelCommon,
    aggregate_rel::Grouping,
    read_rel::{NamedTable, ReadType},
    rel::RelType,
    rel_common::{Emit, EmitKind},
    r#type,
};
use yuzu_anf::anf::{Ident, Rel as AnfRel, RelId, SelectItem, Thunk};
use yuzu_types::TypeId;

use crate::emitter::expr::selection;
use crate::emitter::types::nullable;
use crate::emitter::{SubstraitEmitter, Unsupported};

impl SubstraitEmitter<'_> {
    pub(crate) fn emit_rel(&mut self, id: RelId) -> Result<Rel, Unsupported> {
        let anf = self.anf;
        let rel_type = match anf.rel(id) {
            AnfRel::From { relation, ty, .. } => self.emit_from(*relation, *ty),
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

    fn emit_select(&mut self, input: RelId, items: &[SelectItem]) -> Result<RelType, Unsupported> {
        let input_columns = self.row_fields(self.rel_ty(input)).len() as i32;
        let input = self.emit_rel(input)?;
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
        let input = self.emit_rel(input)?;
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
        let input_columns = self.row_fields(self.rel_ty(input)).len() as i32;
        let input = self.emit_rel(input)?;
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
            | AnfRel::Select { ty, .. }
            | AnfRel::Where { ty, .. }
            | AnfRel::Distinct { ty, .. }
            | AnfRel::Drop { ty, .. }
            | AnfRel::Rename { ty, .. }
            | AnfRel::Extend { ty, .. } => *ty,
        }
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
