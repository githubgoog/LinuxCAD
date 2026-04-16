use crate::types::*;
use axum::{extract::State, http::StatusCode, response::Json};
use std::sync::Arc;
use tracing::{debug, instrument};

/// Compute geometry for a single feature
#[instrument(skip(state, request))]
pub async fn compute_single(
    State(state): State<Arc<crate::AppState>>,
    Json(request): Json<Feature>,
) -> Result<Json<GeometryResponse>, StatusCode> {
    debug!("Computing geometry for feature: {}", request.name);

    match state.geometry_engine.compute_mesh(&request) {
        Ok(mesh) => {
            if let Err(validation_error) = mesh.validate() {
                debug!(
                    "Computed mesh for '{}' failed validation: {}",
                    request.name, validation_error
                );

                return Ok(Json(GeometryResponse {
                    feature_id: request.id.clone(),
                    ok: false,
                    mesh: None,
                    error: Some(format!("Invalid mesh generated: {}", validation_error)),
                }));
            }

            debug!(
                "Successfully computed mesh for '{}': {} vertices, {} faces",
                request.name,
                mesh.vertex_count(),
                mesh.face_count()
            );

            Ok(Json(GeometryResponse {
                feature_id: request.id.clone(),
                ok: true,
                mesh: Some(mesh),
                error: None,
            }))
        }
        Err(e) => {
            debug!("Geometry computation failed for '{}': {}", request.name, e);

            Ok(Json(GeometryResponse {
                feature_id: request.id.clone(),
                ok: false,
                mesh: None,
                error: Some(e.to_string()),
            }))
        }
    }
}

/// Compute geometry for multiple features in batch
#[instrument(skip(state, request))]
pub async fn compute_batch(
    State(state): State<Arc<crate::AppState>>,
    Json(request): Json<GeometryRequest>,
) -> Result<Json<BatchGeometryResponse>, StatusCode> {
    debug!(
        "Computing batch geometry for {} features",
        request.features.len()
    );

    let mut results = Vec::with_capacity(request.features.len());

    for feature in request.features {
        let response = match state.geometry_engine.compute_mesh(&feature) {
            Ok(mesh) => {
                if let Err(validation_error) = mesh.validate() {
                    GeometryResponse {
                        feature_id: feature.id.clone(),
                        ok: false,
                        mesh: None,
                        error: Some(format!("Invalid mesh generated: {}", validation_error)),
                    }
                } else {
                    GeometryResponse {
                        feature_id: feature.id.clone(),
                        ok: true,
                        mesh: Some(mesh),
                        error: None,
                    }
                }
            }
            Err(e) => GeometryResponse {
                feature_id: feature.id.clone(),
                ok: false,
                mesh: None,
                error: Some(e.to_string()),
            },
        };

        results.push(response);
    }

    debug!("Completed batch geometry computation");

    Ok(Json(BatchGeometryResponse { results }))
}

/// Perform shell operation on a feature
#[instrument(skip(state, request))]
pub async fn shell_operation(
    State(state): State<Arc<crate::AppState>>,
    Json(request): Json<ShellRequest>,
) -> Result<Json<GeometryResponse>, StatusCode> {
    debug!(
        "Performing shell operation on feature '{}' with thickness {}",
        request.feature.name, request.thickness
    );

    match state
        .geometry_engine
        .shell_feature(&request.feature, request.thickness)
    {
        Ok(mesh) => Ok(Json(GeometryResponse {
            feature_id: request.feature.id.clone(),
            ok: true,
            mesh: Some(mesh),
            error: None,
        })),
        Err(err) => Ok(Json(GeometryResponse {
            feature_id: request.feature.id.clone(),
            ok: false,
            mesh: None,
            error: Some(err.to_string()),
        })),
    }
}

/// Perform cut operation on a feature
#[instrument(skip(state, request))]
pub async fn cut_operation(
    State(state): State<Arc<crate::AppState>>,
    Json(request): Json<CutRequest>,
) -> Result<Json<GeometryResponse>, StatusCode> {
    debug!(
        "Performing cut operation on '{}' axis={} offset={} keepSide={}",
        request.feature.name, request.axis, request.offset, request.keep_side
    );

    match state.geometry_engine.cut_feature(
        &request.feature,
        &request.axis,
        request.offset,
        &request.keep_side,
    ) {
        Ok(mesh) => Ok(Json(GeometryResponse {
            feature_id: request.feature.id.clone(),
            ok: true,
            mesh: Some(mesh),
            error: None,
        })),
        Err(err) => Ok(Json(GeometryResponse {
            feature_id: request.feature.id.clone(),
            ok: false,
            mesh: None,
            error: Some(err.to_string()),
        })),
    }
}

/// Perform revolve operation on a feature
#[instrument(skip(state, request))]
pub async fn revolve_operation(
    State(state): State<Arc<crate::AppState>>,
    Json(request): Json<RevolveRequest>,
) -> Result<Json<GeometryResponse>, StatusCode> {
    debug!(
        "Performing revolve operation on '{}' axis={} side={} distance={} angle={} segments={}",
        request.feature.name,
        request.axis,
        request.side,
        request.distance,
        request.angle,
        request.segments
    );

    match state.geometry_engine.revolve_feature(
        &request.feature,
        &request.axis,
        &request.side,
        request.distance,
        request.angle,
        request.segments,
    ) {
        Ok(mesh) => Ok(Json(GeometryResponse {
            feature_id: request.feature.id.clone(),
            ok: true,
            mesh: Some(mesh),
            error: None,
        })),
        Err(err) => Ok(Json(GeometryResponse {
            feature_id: request.feature.id.clone(),
            ok: false,
            mesh: None,
            error: Some(err.to_string()),
        })),
    }
}

/// Perform sweep operation on a feature
#[instrument(skip(state, request))]
pub async fn sweep_operation(
    State(state): State<Arc<crate::AppState>>,
    Json(request): Json<SweepRequest>,
) -> Result<Json<GeometryResponse>, StatusCode> {
    debug!(
        "Performing sweep operation on '{}' axis={} side={} distance={} start={} end={} pathRotate={} twist={} segments={}",
        request.feature.name,
        request.axis,
        request.side,
        request.distance,
        request.start_angle,
        request.end_angle,
        request.path_rotate,
        request.twist,
        request.segments
    );

    match state.geometry_engine.sweep_feature(
        &request.feature,
        &request.axis,
        &request.side,
        request.distance,
        request.start_angle,
        request.end_angle,
        request.path_rotate,
        request.twist,
        request.segments,
    ) {
        Ok(mesh) => Ok(Json(GeometryResponse {
            feature_id: request.feature.id.clone(),
            ok: true,
            mesh: Some(mesh),
            error: None,
        })),
        Err(err) => Ok(Json(GeometryResponse {
            feature_id: request.feature.id.clone(),
            ok: false,
            mesh: None,
            error: Some(err.to_string()),
        })),
    }
}

/// Perform boolean operation (placeholder)
#[instrument(skip(state, request))]
pub async fn boolean_operation(
    State(state): State<Arc<crate::AppState>>,
    Json(request): Json<BooleanRequest>,
) -> Result<Json<GeometryResponse>, StatusCode> {
    debug!(
        "Boolean operation requested op={:?} on '{}' and '{}'",
        request.op, request.feature_a.name, request.feature_b.name
    );

    let mesh_a = match state.geometry_engine.compute_mesh(&request.feature_a) {
        Ok(mesh) => mesh,
        Err(err) => {
            return Ok(Json(GeometryResponse {
                feature_id: request.feature_a.id.clone(),
                ok: false,
                mesh: None,
                error: Some(format!("Failed to compute feature A mesh: {}", err)),
            }));
        }
    };

    let mesh_b = match state.geometry_engine.compute_mesh(&request.feature_b) {
        Ok(mesh) => mesh,
        Err(err) => {
            return Ok(Json(GeometryResponse {
                feature_id: request.feature_a.id.clone(),
                ok: false,
                mesh: None,
                error: Some(format!("Failed to compute feature B mesh: {}", err)),
            }));
        }
    };

    let result = match request.op {
        BooleanOp::Union => state.boolean_engine.union(&mesh_a, &mesh_b),
        BooleanOp::Subtract => state.boolean_engine.subtract(&mesh_a, &mesh_b),
        BooleanOp::Intersect => state.boolean_engine.intersect(&mesh_a, &mesh_b),
    };

    match result {
        Ok(mesh) => Ok(Json(GeometryResponse {
            feature_id: request.feature_a.id.clone(),
            ok: true,
            mesh: Some(mesh),
            error: None,
        })),
        Err(err) => Ok(Json(GeometryResponse {
            feature_id: request.feature_a.id.clone(),
            ok: false,
            mesh: None,
            error: Some(err.to_string()),
        })),
    }
}

/// Perform fillet operation (placeholder)
#[instrument(skip(state, request))]
pub async fn fillet_operation(
    State(state): State<Arc<crate::AppState>>,
    Json(request): Json<FilletRequest>,
) -> Result<Json<GeometryResponse>, StatusCode> {
    debug!(
        "Fillet operation requested feature='{}' radius={} iterations={}",
        request.feature.name, request.radius, request.iterations
    );

    match state
        .geometry_engine
        .fillet_feature(&request.feature, request.radius, request.iterations)
    {
        Ok(mesh) => Ok(Json(GeometryResponse {
            feature_id: request.feature.id.clone(),
            ok: true,
            mesh: Some(mesh),
            error: None,
        })),
        Err(err) => Ok(Json(GeometryResponse {
            feature_id: request.feature.id.clone(),
            ok: false,
            mesh: None,
            error: Some(err.to_string()),
        })),
    }
}

/// Perform chamfer operation (placeholder)
#[instrument(skip(state, request))]
pub async fn chamfer_operation(
    State(state): State<Arc<crate::AppState>>,
    Json(request): Json<ChamferRequest>,
) -> Result<Json<GeometryResponse>, StatusCode> {
    debug!(
        "Chamfer operation requested feature='{}' amount={}",
        request.feature.name, request.amount
    );

    match state
        .geometry_engine
        .chamfer_feature(&request.feature, request.amount)
    {
        Ok(mesh) => Ok(Json(GeometryResponse {
            feature_id: request.feature.id.clone(),
            ok: true,
            mesh: Some(mesh),
            error: None,
        })),
        Err(err) => Ok(Json(GeometryResponse {
            feature_id: request.feature.id.clone(),
            ok: false,
            mesh: None,
            error: Some(err.to_string()),
        })),
    }
}

/// Perform loft operation on two features
#[instrument(skip(state, request))]
pub async fn loft_operation(
    State(state): State<Arc<crate::AppState>>,
    Json(request): Json<LoftRequest>,
) -> Result<Json<GeometryResponse>, StatusCode> {
    debug!(
        "Loft operation requested between '{}' and '{}' sections={} affectRatio={}",
        request.feature_a.name, request.feature_b.name, request.sections, request.affect_ratio
    );

    match state.geometry_engine.loft_features(
        &request.feature_a,
        &request.feature_b,
        request.sections,
        request.affect_ratio,
    ) {
        Ok(mesh) => Ok(Json(GeometryResponse {
            feature_id: request.feature_a.id.clone(),
            ok: true,
            mesh: Some(mesh),
            error: None,
        })),
        Err(err) => Ok(Json(GeometryResponse {
            feature_id: request.feature_a.id.clone(),
            ok: false,
            mesh: None,
            error: Some(err.to_string()),
        })),
    }
}
