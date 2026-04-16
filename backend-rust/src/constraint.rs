use crate::types::*;
use std::collections::HashMap;
use tracing::{debug, instrument, warn};

/// Simplified iterative constraint solver
pub struct ConstraintSolver {
    variables: Vec<Variable>,
    constraints: Vec<SolverConstraint>,
    tolerance: f64,
    max_iterations: u32,
}

/// Result of applying a single constraint
struct ConstraintUpdate {
    error: f64,
    updates: HashMap<String, f64>, // Variable ID -> delta
}

impl ConstraintSolver {
    pub fn new() -> Self {
        Self {
            variables: Vec::new(),
            constraints: Vec::new(),
            tolerance: 1e-8,
            max_iterations: 1000,
        }
    }

    /// Build constraint system from sketch
    #[instrument(skip(sketch))]
    pub fn from_sketch(sketch: &Sketch) -> Self {
        let mut solver = Self::new();

        // Add point coordinates as variables
        for point in &sketch.points {
            if !point.construction {
                solver.add_variable(Variable {
                    id: format!("{}.x", point.id),
                    value: point.x,
                    min_value: None,
                    max_value: None,
                    fixed: false,
                });

                solver.add_variable(Variable {
                    id: format!("{}.y", point.id),
                    value: point.y,
                    min_value: None,
                    max_value: None,
                    fixed: false,
                });
            }
        }

        // Add entity parameters as variables
        for entity in &sketch.entities {
            match entity {
                SketchEntity::Circle(circle) => {
                    solver.add_variable(Variable {
                        id: format!("{}.radius", circle.id),
                        value: circle.radius,
                        min_value: Some(0.001),
                        max_value: None,
                        fixed: false,
                    });
                }
                _ => {} // Add other entity types as needed
            }
        }

        // Convert constraints
        for constraint in &sketch.constraints {
            if let Some(solver_constraint) = Self::convert_constraint(constraint, sketch) {
                solver.add_constraint(solver_constraint);
            }
        }

        solver
    }

    pub fn add_variable(&mut self, variable: Variable) {
        self.variables.push(variable);
    }

    pub fn add_constraint(&mut self, constraint: SolverConstraint) {
        self.constraints.push(constraint);
    }

    /// Solve constraints using simplified iterative algorithm
    #[instrument(skip(self))]
    pub fn solve(&self) -> Result<SolverResult, ConstraintError> {
        if self.variables.is_empty() {
            return Ok(SolverResult {
                success: true,
                iterations: 0,
                residual: 0.0,
                variables: HashMap::new(),
                constraint_errors: HashMap::new(),
            });
        }

        // Create mutable copy of variables for iteration
        let mut current_variables: HashMap<String, f64> = self
            .variables
            .iter()
            .map(|v| (v.id.clone(), v.value))
            .collect();
        let variable_metadata: HashMap<&str, &Variable> =
            self.variables.iter().map(|v| (v.id.as_str(), v)).collect();

        let mut best_residual = f64::INFINITY;
        let mut iterations = 0;

        for iteration in 0..self.max_iterations {
            iterations = iteration + 1;
            let mut total_residual = 0.0;
            let mut variable_updates: HashMap<String, f64> = HashMap::new();

            // Apply constraints iteratively
            for constraint in &self.constraints {
                let update = self.apply_constraint_update(constraint, &current_variables);

                // Accumulate updates
                for (var_id, delta) in update.updates {
                    *variable_updates.entry(var_id).or_insert(0.0) += delta * 0.1;
                    // Damping factor
                }

                total_residual += update.error;
            }

            // Apply variable updates
            for (var_id, delta) in variable_updates {
                if let Some(value) = current_variables.get_mut(&var_id) {
                    // Apply bounds if they exist
                    if let Some(variable) = variable_metadata.get(var_id.as_str()) {
                        if !variable.fixed {
                            *value += delta;

                            // Clamp to bounds
                            if let Some(min) = variable.min_value {
                                *value = value.max(min);
                            }
                            if let Some(max) = variable.max_value {
                                *value = value.min(max);
                            }
                        }
                    }
                }
            }

            // Check convergence
            if total_residual < self.tolerance {
                best_residual = total_residual;
                break;
            }

            if total_residual < best_residual {
                best_residual = total_residual;
            }
        }

        // Calculate final constraint errors
        let constraint_errors = self.evaluate_constraint_errors(&current_variables);
        let success = best_residual < self.tolerance * 10.0; // Allow some tolerance

        debug!(
            "Constraint solver completed: {} iterations, residual: {:.2e}, success: {}",
            iterations, best_residual, success
        );

        Ok(SolverResult {
            success,
            iterations,
            residual: best_residual,
            variables: current_variables,
            constraint_errors,
        })
    }

    /// Apply a single constraint and return suggested updates
    fn apply_constraint_update(
        &self,
        constraint: &SolverConstraint,
        variables: &HashMap<String, f64>,
    ) -> ConstraintUpdate {
        match constraint.constraint_type.as_str() {
            "coincident" => {
                if constraint.variables.len() >= 4 {
                    let x1 = *variables.get(&constraint.variables[0]).unwrap_or(&0.0);
                    let y1 = *variables.get(&constraint.variables[1]).unwrap_or(&0.0);
                    let x2 = *variables.get(&constraint.variables[2]).unwrap_or(&0.0);
                    let y2 = *variables.get(&constraint.variables[3]).unwrap_or(&0.0);

                    let dx = x2 - x1;
                    let dy = y2 - y1;
                    let dist_sq = dx * dx + dy * dy;
                    let tolerance_sq = self.tolerance * self.tolerance;

                    if dist_sq > tolerance_sq {
                        let error = dist_sq.sqrt();
                        let mut updates = HashMap::new();
                        updates.insert(constraint.variables[0].clone(), dx * 0.5);
                        updates.insert(constraint.variables[1].clone(), dy * 0.5);
                        updates.insert(constraint.variables[2].clone(), -dx * 0.5);
                        updates.insert(constraint.variables[3].clone(), -dy * 0.5);

                        return ConstraintUpdate { error, updates };
                    }
                }
            }

            "horizontal" => {
                if constraint.variables.len() >= 2 {
                    let y1 = *variables.get(&constraint.variables[0]).unwrap_or(&0.0);
                    let y2 = *variables.get(&constraint.variables[1]).unwrap_or(&0.0);
                    let error = (y2 - y1).abs();

                    if error > self.tolerance {
                        let avg_y = (y1 + y2) * 0.5;
                        let mut updates = HashMap::new();
                        updates.insert(constraint.variables[0].clone(), avg_y - y1);
                        updates.insert(constraint.variables[1].clone(), avg_y - y2);

                        return ConstraintUpdate { error, updates };
                    }
                }
            }

            "vertical" => {
                if constraint.variables.len() >= 2 {
                    let x1 = *variables.get(&constraint.variables[0]).unwrap_or(&0.0);
                    let x2 = *variables.get(&constraint.variables[1]).unwrap_or(&0.0);
                    let error = (x2 - x1).abs();

                    if error > self.tolerance {
                        let avg_x = (x1 + x2) * 0.5;
                        let mut updates = HashMap::new();
                        updates.insert(constraint.variables[0].clone(), avg_x - x1);
                        updates.insert(constraint.variables[1].clone(), avg_x - x2);

                        return ConstraintUpdate { error, updates };
                    }
                }
            }

            "distance" => {
                if constraint.variables.len() >= 4 && constraint.target_value.is_some() {
                    let x1 = *variables.get(&constraint.variables[0]).unwrap_or(&0.0);
                    let y1 = *variables.get(&constraint.variables[1]).unwrap_or(&0.0);
                    let x2 = *variables.get(&constraint.variables[2]).unwrap_or(&0.0);
                    let y2 = *variables.get(&constraint.variables[3]).unwrap_or(&0.0);

                    let dx = x2 - x1;
                    let dy = y2 - y1;
                    let current_distance = (dx * dx + dy * dy).sqrt();
                    let target_distance = constraint.target_value.unwrap();
                    let error = (current_distance - target_distance).abs();

                    if error > self.tolerance && current_distance > 1e-10 {
                        let scale = target_distance / current_distance;
                        let center_x = (x1 + x2) * 0.5;
                        let center_y = (y1 + y2) * 0.5;

                        let new_x1 = center_x - dx * scale * 0.5;
                        let new_y1 = center_y - dy * scale * 0.5;
                        let new_x2 = center_x + dx * scale * 0.5;
                        let new_y2 = center_y + dy * scale * 0.5;

                        let mut updates = HashMap::new();
                        updates.insert(constraint.variables[0].clone(), new_x1 - x1);
                        updates.insert(constraint.variables[1].clone(), new_y1 - y1);
                        updates.insert(constraint.variables[2].clone(), new_x2 - x2);
                        updates.insert(constraint.variables[3].clone(), new_y2 - y2);

                        return ConstraintUpdate { error, updates };
                    }
                }
            }

            _ => {}
        }

        ConstraintUpdate {
            error: 0.0,
            updates: HashMap::new(),
        }
    }

    fn convert_constraint(
        constraint: &GeometricConstraint,
        sketch: &Sketch,
    ) -> Option<SolverConstraint> {
        match constraint.constraint_type {
            ConstraintType::Coincident => {
                if let Some(point_ids) = &constraint.point_ids {
                    if point_ids.len() >= 2 {
                        let p1 = point_ids[0];
                        let p2 = point_ids[1];
                        return Some(SolverConstraint {
                            id: constraint.id.to_string(),
                            constraint_type: "coincident".to_string(),
                            variables: vec![
                                format!("{}.x", p1),
                                format!("{}.y", p1),
                                format!("{}.x", p2),
                                format!("{}.y", p2),
                            ],
                            target_value: None,
                            weight: 1.0,
                        });
                    }
                }
            }

            ConstraintType::Horizontal => {
                if !constraint.entity_ids.is_empty() {
                    let entity_id = constraint.entity_ids[0];
                    if let Some(SketchEntity::Line(line)) = sketch.entities.iter().find(|e| match e
                    {
                        SketchEntity::Line(l) => l.id == entity_id,
                        _ => false,
                    }) {
                        return Some(SolverConstraint {
                            id: constraint.id.to_string(),
                            constraint_type: "horizontal".to_string(),
                            variables: vec![
                                format!("{}.y", line.start_point_id),
                                format!("{}.y", line.end_point_id),
                            ],
                            target_value: None,
                            weight: 1.0,
                        });
                    }
                }
            }

            ConstraintType::Vertical => {
                if !constraint.entity_ids.is_empty() {
                    let entity_id = constraint.entity_ids[0];
                    if let Some(SketchEntity::Line(line)) = sketch.entities.iter().find(|e| match e
                    {
                        SketchEntity::Line(l) => l.id == entity_id,
                        _ => false,
                    }) {
                        return Some(SolverConstraint {
                            id: constraint.id.to_string(),
                            constraint_type: "vertical".to_string(),
                            variables: vec![
                                format!("{}.x", line.start_point_id),
                                format!("{}.x", line.end_point_id),
                            ],
                            target_value: None,
                            weight: 1.0,
                        });
                    }
                }
            }

            ConstraintType::Distance => {
                if let Some(point_ids) = &constraint.point_ids {
                    if point_ids.len() >= 2 && constraint.value.is_some() {
                        let p1 = point_ids[0];
                        let p2 = point_ids[1];
                        return Some(SolverConstraint {
                            id: constraint.id.to_string(),
                            constraint_type: "distance".to_string(),
                            variables: vec![
                                format!("{}.x", p1),
                                format!("{}.y", p1),
                                format!("{}.x", p2),
                                format!("{}.y", p2),
                            ],
                            target_value: constraint.value,
                            weight: 1.0,
                        });
                    }
                }
            }

            // Add more constraint types...
            _ => {
                warn!(
                    "Unsupported constraint type: {:?}",
                    constraint.constraint_type
                );
                return None;
            }
        }

        None
    }

    fn evaluate_constraint_errors(&self, variables: &HashMap<String, f64>) -> HashMap<String, f64> {
        let mut errors = HashMap::new();

        for constraint in &self.constraints {
            let error = self.evaluate_constraint_error(constraint, variables);
            errors.insert(constraint.id.clone(), error);
        }

        errors
    }

    fn evaluate_constraint_error(
        &self,
        constraint: &SolverConstraint,
        variables: &HashMap<String, f64>,
    ) -> f64 {
        match constraint.constraint_type.as_str() {
            "coincident" => {
                if constraint.variables.len() >= 4 {
                    let x1 = variables.get(&constraint.variables[0]).unwrap_or(&0.0);
                    let y1 = variables.get(&constraint.variables[1]).unwrap_or(&0.0);
                    let x2 = variables.get(&constraint.variables[2]).unwrap_or(&0.0);
                    let y2 = variables.get(&constraint.variables[3]).unwrap_or(&0.0);
                    ((x2 - x1).powi(2) + (y2 - y1).powi(2)).sqrt()
                } else {
                    0.0
                }
            }

            "horizontal" => {
                if constraint.variables.len() >= 2 {
                    let y1 = variables.get(&constraint.variables[0]).unwrap_or(&0.0);
                    let y2 = variables.get(&constraint.variables[1]).unwrap_or(&0.0);
                    (y2 - y1).abs()
                } else {
                    0.0
                }
            }

            "vertical" => {
                if constraint.variables.len() >= 2 {
                    let x1 = variables.get(&constraint.variables[0]).unwrap_or(&0.0);
                    let x2 = variables.get(&constraint.variables[1]).unwrap_or(&0.0);
                    (x2 - x1).abs()
                } else {
                    0.0
                }
            }

            "distance" => {
                if constraint.variables.len() >= 4 && constraint.target_value.is_some() {
                    let x1 = variables.get(&constraint.variables[0]).unwrap_or(&0.0);
                    let y1 = variables.get(&constraint.variables[1]).unwrap_or(&0.0);
                    let x2 = variables.get(&constraint.variables[2]).unwrap_or(&0.0);
                    let y2 = variables.get(&constraint.variables[3]).unwrap_or(&0.0);
                    let distance = ((x2 - x1).powi(2) + (y2 - y1).powi(2)).sqrt();
                    (distance - constraint.target_value.unwrap()).abs()
                } else {
                    0.0
                }
            }

            _ => 0.0,
        }
    }
}

impl Default for ConstraintSolver {
    fn default() -> Self {
        Self::new()
    }
}

/// Apply constraint solver results to a sketch
pub fn apply_solution_to_sketch(sketch: &mut Sketch, result: &SolverResult) {
    // Update point coordinates
    for point in &mut sketch.points {
        if let Some(&x) = result.variables.get(&format!("{}.x", point.id)) {
            point.x = x;
        }
        if let Some(&y) = result.variables.get(&format!("{}.y", point.id)) {
            point.y = y;
        }
    }

    // Update entity parameters
    for entity in &mut sketch.entities {
        match entity {
            SketchEntity::Circle(circle) => {
                if let Some(&radius) = result.variables.get(&format!("{}.radius", circle.id)) {
                    circle.radius = radius;
                }
            }
            _ => {} // Handle other entity types
        }
    }

    // Update constraint satisfaction status
    for constraint in &mut sketch.constraints {
        if let Some(&error) = result.constraint_errors.get(&constraint.id.to_string()) {
            constraint.satisfied = error < 1e-6;
        }
    }

    // Check if sketch is fully constrained
    sketch.fully_constrained = sketch
        .constraints
        .iter()
        .filter(|c| c.enabled)
        .all(|c| c.satisfied);
}
