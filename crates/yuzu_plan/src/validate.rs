use std::collections::HashSet;

use crate::target::{Target, TargetRegistry};
use yuzu_diagnostics::diagnostics::{Span, builder::DiagnosticBuilder, engine::DiagnosticsEngine};

use crate::graph::RelGraph;
use crate::{Expr, ExprId, JoinCondition, Measure, Rel};

/// Checks every function the graph computes against what the target can
/// execute. The graph is well-formed by construction; whether the chosen
/// target runs it is a property of the target, answered here — emission stays
/// free of capability knowledge.
pub fn validate(
    graph: &RelGraph,
    target: &Target,
    diagnostics: &mut DiagnosticsEngine,
    query_span: Span,
) {
    let registry = target.registry();
    let mut validator = Validator {
        graph,
        target,
        registry: registry.as_ref(),
        diagnostics,
        query_span,
        visited: HashSet::new(),
    };
    for node in graph.nodes() {
        validator.check_rel(node);
    }
}

struct Validator<'v> {
    graph: &'v RelGraph,
    target: &'v Target,
    registry: &'v dyn TargetRegistry,
    diagnostics: &'v mut DiagnosticsEngine,
    query_span: Span,
    visited: HashSet<ExprId>,
}

impl Validator<'_> {
    fn check_rel(&mut self, id: crate::RelId) {
        match self.graph.plan().rel(id) {
            Rel::Join {
                condition: Some(JoinCondition::On(predicate)),
                ..
            } => self.check_expr(*predicate),
            Rel::Select { items, .. } | Rel::Extend { items, .. } => {
                for item in items.iter() {
                    self.check_expr(item.body);
                }
            }
            Rel::Where { predicate, .. } => self.check_expr(*predicate),
            Rel::Set { items, .. } => {
                for item in items.iter() {
                    self.check_expr(item.value);
                }
            }
            Rel::Aggregate { measures, .. } => {
                for measure in measures.iter() {
                    self.check_measure(measure);
                }
            }
            _ => {}
        }
    }

    fn check_measure(&mut self, measure: &Measure) {
        if !self.target.supports(self.registry.aggregate(measure.func)) {
            self.report(measure.func.name());
        }
        for &arg in measure.args.iter() {
            self.check_expr(arg);
        }
    }

    fn check_expr(&mut self, id: ExprId) {
        if !self.visited.insert(id) {
            return;
        }
        if let Expr::Call { func, args, .. } = self.graph.plan().expr(id) {
            if !self.target.supports(self.registry.scalar(*func)) {
                self.report(func.symbol());
            }
            for &arg in args.iter() {
                self.check_expr(arg);
            }
        }
    }

    fn report(&mut self, name: &str) {
        let message = format!("`{name}` is not supported by the {} target", self.target);
        self.diagnostics
            .emit(DiagnosticBuilder::error(self.query_span, message));
    }
}
