use crate::boolean::BooleanEngine;
use crate::types::*;
use axum::{extract::State, http::StatusCode, response::Json};
use serde::{Deserialize, Serialize};
use std::sync::Arc;
use tracing::{debug, error, instrument};

/// Request for boolean operation
#[derive(Debug, Deserialize)]
pub struct BooleanRequest {
    pub operation: BooleanOperation,
    pub source_feature: Feature,
    pub target_feature: Feature,
}

/// Response for boolean operation
#[derive(Debug, Serialize)]
pub struct BooleanResponse {
    pub ok: bool,
    pub mesh: Option<MeshData>,
    pub error: Option<String>,
    pub operation_type: String,
}

/// Boolean operation type
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "lowercase")]
pub enum BooleanOperation {
    Union,
    Subtract,
    Intersect,
}

/// Combined boolean operation endpoint (supports all types in one endpoint)
#[instrument(skip(state, request))]
pub async fn boolean_operation(
    State(state): State<Arc<crate::AppState>>,
    Json(request): Json<BooleanRequest>,
) -> Result<Json<BooleanResponse>, StatusCode> {
    let operation_name = match &request.operation {
        BooleanOperation::Union => "union",
        BooleanOperation::Subtract => "subtract",
        BooleanOperation::Intersect => "intersect",
    };

    debug!("Boolean {} operation requested", operation_name);

    match perform_boolean_operation(&state, request.source_feature, request.target_feature, request.operation.clone()).await {
        Ok(mesh) => Ok(Json(BooleanResponse {
            ok: true,
            mesh: Some(mesh),
            error: None,
            operation_type: operation_name.to_string(),
        })),
        Err(e) => {
            error!("Boolean {} operation failed: {}", operation_name, e);
            Ok(Json(BooleanResponse {
                ok: false,
                mesh: None,
                error: Some(e),
                operation_type: operation_name.to_string(),
            }))
        }
    }
}

/// Generic boolean operation handler
async fn perform_boolean_operation(
    state: &Arc<crate::AppState>,
    source_feature: Feature,
    target_feature: Feature,
    operation: BooleanOperation,
) -> Result<MeshData, String> {
    let geometry_engine = crate::geometry::GeometryEngine::new();
    let boolean_engine = BooleanEngine::new(1e-6);

    debug!("Computing source mesh for feature: {}", source_feature.id);
    let source_mesh = geometry_engine.compute_mesh(&source_feature)
        .map_err(|e| format!("Failed to compute source mesh: {}", e))?;

    debug!("Computing target mesh for feature: {}", target_feature.id);
    let target_mesh = geometry_engine.compute_mesh(&target_feature)
        .map_err(|e| format!("Failed to compute target mesh: {}", e))?;

    debug!("Performing {:?} operation", operation);
    let result_mesh = match operation {
        BooleanOperation::Union => boolean_engine.union(&source_mesh, &target_mesh),
        BooleanOperation::Subtract => boolean_engine.subtract(&source_mesh, &target_mesh),
        BooleanOperation::Intersect => boolean_engine.intersect(&source_mesh, &target_mesh),
    }.map_err(|e| format!("Boolean operation failed: {}", e))?;

    debug!("Boolean operation completed successfully");
    Ok(result_mesh)
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::collections::HashMap;

    fn create_test_feature(name: &str, primitive_type: PrimitiveType) -> Feature {
        let mut params = HashMap::new();
        params.insert("width".to_string(), 2.0);
        params.insert("height".to_string(), 2.0);
        params.insert("depth".to_string(), 2.0);

        Feature {
            id: format!("test-{}", name),
            feature_type: FeatureType::Primitive(primitive_type),
            name: name.to_string(),
            assembly_id: None,
            visible: true,
            color: "#ff0000".to_string(),
            params,
            source_ids: None,
            baked_geometry: None,
        }
    }

    #[tokio::test]
    async fn test_boolean_request_serialization() {
        let source = create_test_feature("source", PrimitiveType::Box);
        let target = create_test_feature("target", PrimitiveType::Sphere);

        let request = BooleanRequest {
            operation: BooleanOperation::Union,
            source_feature: source,
            target_feature: target,
        };

        let serialized = serde_json::to_string(&request).unwrap();
        assert!(serialized.contains("union"));
        assert!(serialized.contains("test-source"));
        assert!(serialized.contains("test-target"));
    }

    #[tokio::test]
    async fn test_boolean_operation_types() {
        // Test all operation types serialize correctly
        let operations = vec![
            BooleanOperation::Union,
            BooleanOperation::Subtract,
            BooleanOperation::Intersect,
        ];

        for op in operations {
            let serialized = serde_json::to_string(&op).unwrap();
            match op {
                BooleanOperation::Union => assert_eq!(serialized, "\"union\""),
                BooleanOperation::Subtract => assert_eq!(serialized, "\"subtract\""),
                BooleanOperation::Intersect => assert_eq!(serialized, "\"intersect\""),
            }
        }
    }
}