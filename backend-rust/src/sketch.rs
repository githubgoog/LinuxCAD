use crate::{constraint::*, types::*};
use nalgebra::Point2;
use tracing::{debug, instrument};
use uuid::Uuid;

/// High-performance 2D sketching system
pub struct SketchEngine {
    // Could add caching or optimization state here
}

impl SketchEngine {
    pub fn new() -> Self {
        Self {}
    }

    /// Solve constraints for a sketch using the high-performance constraint solver
    #[instrument(skip(self, sketch))]
    pub fn solve_constraints(&self, sketch: &mut Sketch) -> Result<SolverResult, ConstraintError> {
        debug!("Solving constraints for sketch: {}", sketch.name);

        // Create constraint solver from sketch
        let solver = ConstraintSolver::from_sketch(sketch);

        // Solve the constraint system
        let result = solver.solve()?;

        // Apply results back to sketch
        apply_solution_to_sketch(sketch, &result);

        debug!(
            "Constraint solving completed: {} iterations, residual: {:.2e}, success: {}",
            result.iterations, result.residual, result.success
        );

        Ok(result)
    }

    /// Validate a sketch for geometric consistency
    #[instrument(skip(self, sketch))]
    pub fn validate_sketch(&self, sketch: &Sketch) -> SketchValidationResult {
        let mut result = SketchValidationResult {
            valid: true,
            errors: Vec::new(),
            warnings: Vec::new(),
        };

        // Check for duplicate points
        self.check_duplicate_points(sketch, &mut result);

        // Check for invalid entities
        self.check_invalid_entities(sketch, &mut result);

        // Check for impossible constraints
        self.check_impossible_constraints(sketch, &mut result);

        // Check for over-constrained system
        self.check_over_constrained(sketch, &mut result);

        result
    }

    /// Create a new line entity between two points
    pub fn create_line(
        &self,
        start: SketchPoint,
        end: SketchPoint,
    ) -> (Vec<SketchPoint>, SketchEntity) {
        let line = SketchLine {
            id: Uuid::new_v4(),
            start_point_id: start.id,
            end_point_id: end.id,
            construction: false,
            visible: true,
        };

        (vec![start, end], SketchEntity::Line(line))
    }

    /// Create a new circle entity
    pub fn create_circle(
        &self,
        center: SketchPoint,
        radius: f64,
    ) -> (Vec<SketchPoint>, SketchEntity) {
        let circle = SketchCircle {
            id: Uuid::new_v4(),
            center_point_id: center.id,
            radius,
            construction: false,
            visible: true,
        };

        (vec![center], SketchEntity::Circle(circle))
    }

    /// Add a coincident constraint between two points
    pub fn add_coincident_constraint(
        &self,
        point1_id: Uuid,
        point2_id: Uuid,
    ) -> GeometricConstraint {
        GeometricConstraint {
            id: Uuid::new_v4(),
            constraint_type: ConstraintType::Coincident,
            entity_ids: Vec::new(),
            point_ids: Some(vec![point1_id, point2_id]),
            value: None,
            enabled: true,
            satisfied: false,
        }
    }

    /// Add a horizontal constraint to a line
    pub fn add_horizontal_constraint(&self, line_id: Uuid) -> GeometricConstraint {
        GeometricConstraint {
            id: Uuid::new_v4(),
            constraint_type: ConstraintType::Horizontal,
            entity_ids: vec![line_id],
            point_ids: None,
            value: None,
            enabled: true,
            satisfied: false,
        }
    }

    /// Add a distance constraint between two points
    pub fn add_distance_constraint(
        &self,
        point1_id: Uuid,
        point2_id: Uuid,
        distance: f64,
    ) -> GeometricConstraint {
        GeometricConstraint {
            id: Uuid::new_v4(),
            constraint_type: ConstraintType::Distance,
            entity_ids: Vec::new(),
            point_ids: Some(vec![point1_id, point2_id]),
            value: Some(distance),
            enabled: true,
            satisfied: false,
        }
    }

    // Validation helper methods
    fn check_duplicate_points(&self, sketch: &Sketch, result: &mut SketchValidationResult) {
        for (i, point1) in sketch.points.iter().enumerate() {
            for (_j, point2) in sketch.points.iter().enumerate().skip(i + 1) {
                let distance =
                    ((point1.x - point2.x).powi(2) + (point1.y - point2.y).powi(2)).sqrt();
                if distance < 1e-6 {
                    result.warnings.push(format!(
                        "Points {} and {} are very close (distance: {:.2e})",
                        point1.id, point2.id, distance
                    ));
                }
            }
        }
    }

    fn check_invalid_entities(&self, sketch: &Sketch, result: &mut SketchValidationResult) {
        for entity in &sketch.entities {
            match entity {
                SketchEntity::Line(line) => {
                    if !sketch.points.iter().any(|p| p.id == line.start_point_id) {
                        result.errors.push(format!(
                            "Line {} references non-existent start point {}",
                            line.id, line.start_point_id
                        ));
                        result.valid = false;
                    }
                    if !sketch.points.iter().any(|p| p.id == line.end_point_id) {
                        result.errors.push(format!(
                            "Line {} references non-existent end point {}",
                            line.id, line.end_point_id
                        ));
                        result.valid = false;
                    }
                }
                SketchEntity::Circle(circle) => {
                    if !sketch.points.iter().any(|p| p.id == circle.center_point_id) {
                        result.errors.push(format!(
                            "Circle {} references non-existent center point {}",
                            circle.id, circle.center_point_id
                        ));
                        result.valid = false;
                    }
                    if circle.radius <= 0.0 {
                        result.errors.push(format!(
                            "Circle {} has invalid radius: {}",
                            circle.id, circle.radius
                        ));
                        result.valid = false;
                    }
                }
            }
        }
    }

    fn check_impossible_constraints(&self, sketch: &Sketch, result: &mut SketchValidationResult) {
        for constraint in &sketch.constraints {
            match constraint.constraint_type {
                ConstraintType::Distance => {
                    if let Some(value) = constraint.value {
                        if value < 0.0 {
                            result.errors.push(format!(
                                "Distance constraint {} has negative value: {}",
                                constraint.id, value
                            ));
                            result.valid = false;
                        }
                    }
                }
                _ => {} // Add other constraint validations as needed
            }
        }
    }

    fn check_over_constrained(&self, sketch: &Sketch, result: &mut SketchValidationResult) {
        let num_variables = sketch.points.iter().filter(|p| !p.construction).count() * 2; // Each point has x, y coordinates

        let num_constraints = sketch.constraints.iter().filter(|c| c.enabled).count();

        if num_constraints > num_variables {
            result.warnings.push(format!(
                "Sketch may be over-constrained: {} constraints for {} degrees of freedom",
                num_constraints, num_variables
            ));
        }
    }
}

impl Default for SketchEngine {
    fn default() -> Self {
        Self::new()
    }
}

#[derive(Debug, Clone, serde::Serialize)]
pub struct SketchValidationResult {
    pub valid: bool,
    pub errors: Vec<String>,
    pub warnings: Vec<String>,
}

/// Utility functions for sketch geometry calculations
pub mod sketch_utils {
    use super::*;

    /// Calculate distance between two points
    pub fn point_distance(p1: &SketchPoint, p2: &SketchPoint) -> f64 {
        ((p2.x - p1.x).powi(2) + (p2.y - p1.y).powi(2)).sqrt()
    }

    /// Calculate angle between two lines (in radians)
    pub fn line_angle(
        line1: &SketchLine,
        line2: &SketchLine,
        points: &[SketchPoint],
    ) -> Option<f64> {
        let p1_start = points.iter().find(|p| p.id == line1.start_point_id)?;
        let p1_end = points.iter().find(|p| p.id == line1.end_point_id)?;
        let p2_start = points.iter().find(|p| p.id == line2.start_point_id)?;
        let p2_end = points.iter().find(|p| p.id == line2.end_point_id)?;

        let v1 = Point2::new(p1_end.x - p1_start.x, p1_end.y - p1_start.y);
        let v2 = Point2::new(p2_end.x - p2_start.x, p2_end.y - p2_start.y);

        let len1 = (v1.x.powi(2) + v1.y.powi(2)).sqrt();
        let len2 = (v2.x.powi(2) + v2.y.powi(2)).sqrt();

        if len1 == 0.0 || len2 == 0.0 {
            return None;
        }

        let cos_angle = (v1.x * v2.x + v1.y * v2.y) / (len1 * len2);
        Some(cos_angle.clamp(-1.0, 1.0).acos())
    }

    /// Check if two lines are parallel within tolerance
    pub fn lines_parallel(
        line1: &SketchLine,
        line2: &SketchLine,
        points: &[SketchPoint],
        tolerance: f64,
    ) -> bool {
        if let Some(angle) = line_angle(line1, line2, points) {
            angle.abs() < tolerance || (std::f64::consts::PI - angle.abs()) < tolerance
        } else {
            false
        }
    }

    /// Check if two lines are perpendicular within tolerance
    pub fn lines_perpendicular(
        line1: &SketchLine,
        line2: &SketchLine,
        points: &[SketchPoint],
        tolerance: f64,
    ) -> bool {
        if let Some(angle) = line_angle(line1, line2, points) {
            (angle - std::f64::consts::PI / 2.0).abs() < tolerance
        } else {
            false
        }
    }

    /// Calculate line length
    pub fn line_length(line: &SketchLine, points: &[SketchPoint]) -> Option<f64> {
        let start = points.iter().find(|p| p.id == line.start_point_id)?;
        let end = points.iter().find(|p| p.id == line.end_point_id)?;
        Some(point_distance(start, end))
    }

    /// Find intersection point of two lines (if they intersect)
    pub fn line_intersection(
        line1: &SketchLine,
        line2: &SketchLine,
        points: &[SketchPoint],
    ) -> Option<SketchPoint> {
        let p1 = points.iter().find(|p| p.id == line1.start_point_id)?;
        let p2 = points.iter().find(|p| p.id == line1.end_point_id)?;
        let p3 = points.iter().find(|p| p.id == line2.start_point_id)?;
        let p4 = points.iter().find(|p| p.id == line2.end_point_id)?;

        let d1x = p2.x - p1.x;
        let d1y = p2.y - p1.y;
        let d2x = p4.x - p3.x;
        let d2y = p4.y - p3.y;
        let d3x = p1.x - p3.x;
        let d3y = p1.y - p3.y;

        let det = d1x * d2y - d1y * d2x;

        if det.abs() < 1e-10 {
            return None; // Lines are parallel
        }

        let t1 = (d2x * d3y - d2y * d3x) / det;

        let intersection_x = p1.x + t1 * d1x;
        let intersection_y = p1.y + t1 * d1y;

        Some(SketchPoint::new(intersection_x, intersection_y))
    }
}
