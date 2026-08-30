pub mod anf_converter;

use std::collections::HashMap;

use petgraph::{Directed, Direction, graphmap::GraphMap};

use crate::{Expr, ExprId, PlanCtx, Rel, RelId};

/// Converts one dialect's relational tree into plan nodes — `None` if the
/// source contains something a plan cannot express, reported as a diagnostic
/// at its source position.
pub trait RelGraphConverter<Root> {
    fn convert(&mut self, root: Root) -> Option<RelGraph>;
}

#[derive(Default)]
pub struct RelGraph {
    plan: PlanCtx,
    graph: GraphMap<RelId, u8, Directed>,
    interned: HashMap<(Rel, Box<[RelId]>), RelId>,
    root: Option<RelId>,
}

impl RelGraph {
    pub fn new() -> Self {
        Self::default()
    }

    /// Interns a relation over its inputs, returning the node that stands for
    /// it. An edge's weight is its operand position — weight 0 is the first
    /// input — which is how `inputs` gives them back in order.
    pub fn add(&mut self, rel: Rel, inputs: &[RelId]) -> RelId {
        debug_assert_eq!(rel.arity(), inputs.len());
        let key = (rel, Box::from(inputs));
        if let Some(&id) = self.interned.get(&key) {
            return id;
        }

        let id = self.plan.alloc_rel(key.0.clone());
        self.graph.add_node(id);
        for (position, &input) in inputs.iter().enumerate() {
            self.graph.add_edge(id, input, position as u8);
        }

        self.interned.insert(key, id);
        id
    }

    pub fn intern_expr(&mut self, expr: Expr) -> ExprId {
        self.plan.intern_expr(expr)
    }

    pub fn set_root(&mut self, id: RelId) {
        self.root = Some(id);
    }

    pub fn root(&self) -> Option<RelId> {
        self.root
    }

    pub fn plan(&self) -> &PlanCtx {
        &self.plan
    }

    pub fn node_count(&self) -> usize {
        self.graph.node_count()
    }

    pub fn nodes(&self) -> impl Iterator<Item = RelId> + '_ {
        self.graph.nodes()
    }

    /// What a node reads, in operand order. A binary node over one relation
    /// twice has a single edge, so the arity decides how often it appears.
    pub fn inputs(&self, id: RelId) -> Vec<RelId> {
        let mut edges: Vec<(u8, RelId)> = self
            .graph
            .edges(id)
            .map(|(_, input, &position)| (position, input))
            .collect();
        edges.sort_unstable();

        let mut inputs: Vec<RelId> = edges.into_iter().map(|(_, input)| input).collect();
        if inputs.len() == 1 && self.plan.rel(id).arity() == 2 {
            inputs.push(inputs[0]);
        }
        inputs
    }

    /// The relations that read this one.
    pub fn parents(&self, id: RelId) -> impl Iterator<Item = RelId> + '_ {
        self.graph.neighbors_directed(id, Direction::Incoming)
    }
}
