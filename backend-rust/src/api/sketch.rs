use crate::{sketch::SketchValidationResult, types::*};
use axum::{extract::State, http::StatusCode, response::Json};
use std::sync::Arc;
use tracing::{debug, instrument};

/// Solve constraints for a sketch
#[instrument(skip(state, mut_sketch))]
pub async fn solve_constraints(
    State(state): State<Arc<crate::AppState>>,
    Json(mut mut_sketch): Json<Sketch>,
) -> Result<Json<SolverResult>, StatusCode> {
    debug!(
        "Solving constraints for sketch '{}' with {} constraints",
        mut_sketch.name,
        mut_sketch.constraints.len()
    );

    match state.sketch_engine.solve_constraints(&mut mut_sketch) {
        Ok(result) => {
            debug!(
                "Constraint solving successful: {} iterations, residual: {:.2e}",
                result.iterations, result.residual
            );
            Ok(Json(result))
        }
        Err(e) => {
            debug!("Constraint solving failed: {}", e);
            Err(StatusCode::BAD_REQUEST)
        }
    }
}

/// Validate a sketch for geometric consistency
#[instrument(skip(state, sketch))]
pub async fn validate_sketch(
    State(state): State<Arc<crate::AppState>>,
    Json(sketch): Json<Sketch>,
) -> Result<Json<SketchValidationResult>, StatusCode> {
    debug!("Validating sketch '{}'", sketch.name);

    let validation = state.sketch_engine.validate_sketch(&sketch);

    debug!(
        "Sketch validation complete: valid={}, errors={}, warnings={}",
        validation.valid,
        validation.errors.len(),
        validation.warnings.len()
    );

    Ok(Json(validation))
}

/// Get sketch analysis information
#[instrument(skip(_state, sketch))]
pub async fn analyze_sketch(
    State(_state): State<Arc<crate::AppState>>,
    Json(sketch): Json<Sketch>,
) -> Result<Json<SketchAnalysis>, StatusCode> {
    debug!("Analyzing sketch '{}'", sketch.name);

    let analysis = SketchAnalysis {
        point_count: sketch.points.len(),
        entity_count: sketch.entities.len(),
        constraint_count: sketch.constraints.len(),
        construction_points: sketch.points.iter().filter(|p| p.construction).count(),
        visible_entities: sketch
            .entities
            .iter()
            .filter(|e| match e {
                SketchEntity::Line(l) => l.visible,
                SketchEntity::Circle(c) => c.visible,
            })
            .count(),
        enabled_constraints: sketch.constraints.iter().filter(|c| c.enabled).count(),
        satisfied_constraints: sketch.constraints.iter().filter(|c| c.satisfied).count(),
        fully_constrained: sketch.fully_constrained,
    };

    Ok(Json(analysis))
}

#[derive(serde::Serialize)]
pub struct SketchAnalysis {
    pub point_count: usize,
    pub entity_count: usize,
    pub constraint_count: usize,
    pub construction_points: usize,
    pub visible_entities: usize,
    pub enabled_constraints: usize,
    pub satisfied_constraints: usize,
    pub fully_constrained: bool,
}
