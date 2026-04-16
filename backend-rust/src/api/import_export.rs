use crate::import_export::{import_stl, import_obj, import_ply, export_stl, export_obj, export_ply};
use crate::types::MeshData;
use axum::{
    extract::{Multipart, Path as PathExtract, State},
    http::StatusCode,
    response::{IntoResponse, Json},
};
use serde::{Deserialize, Serialize};
use std::path::PathBuf;
use std::sync::Arc;
use tokio::fs;
use tokio::io::AsyncWriteExt;
use tracing::{debug, error, instrument, warn};

const IMPORT_FORMATS: &[(&str, &str, &str)] = &[
    ("STL", ".stl", "STereoLithography format"),
    ("OBJ", ".obj", "Wavefront OBJ format"),
    ("PLY", ".ply", "Polygon File Format"),
];

const EXPORT_FORMATS: &[(&str, &str, &str)] = &[
    ("STL", ".stl", "STereoLithography format"),
    ("OBJ", ".obj", "Wavefront OBJ format"),
    ("PLY", ".ply", "Polygon File Format"),
];

fn build_formats(formats: &[(&str, &str, &str)]) -> Vec<FileFormat> {
    formats
        .iter()
        .map(|(name, extension, description)| FileFormat {
            name: (*name).to_string(),
            extension: (*extension).to_string(),
            description: (*description).to_string(),
            supported: true, // STL, OBJ, and PLY are now implemented
        })
        .collect()
}

pub fn import_format_names() -> Vec<String> {
    IMPORT_FORMATS.iter().map(|(name, _, _)| (*name).to_string()).collect()
}

pub fn export_format_names() -> Vec<String> {
    EXPORT_FORMATS.iter().map(|(name, _, _)| (*name).to_string()).collect()
}

#[derive(Debug, Deserialize)]
pub struct ImportRequest {
    pub file_path: String,
    pub scale: Option<f32>,
}

#[derive(Debug, Serialize)]
pub struct ImportResponse {
    pub success: bool,
    pub meshes: Option<Vec<MeshData>>,
    pub count: usize,
    pub filename: Option<String>,
    pub error: Option<String>,
}

#[derive(Debug, Deserialize)]
pub struct ExportRequest {
    pub file_path: String,
    pub meshes: Vec<MeshData>,
    pub format: String,
    pub binary: Option<bool>,
}

#[derive(Debug, Serialize)]
pub struct ExportResponse {
    pub success: bool,
    pub file_path: Option<String>,
    pub error: Option<String>,
}

/// Import a mesh file (STL, OBJ, PLY)
#[instrument(skip(_state))]
pub async fn import_mesh(
    State(_state): State<Arc<crate::AppState>>,
    Json(request): Json<ImportRequest>,
) -> Result<Json<ImportResponse>, StatusCode> {
    debug!("Mesh import requested: {}", request.file_path);

    let path = PathBuf::from(&request.file_path);

    if !path.exists() {
        warn!("File not found: {}", request.file_path);
        return Ok(Json(ImportResponse {
            success: false,
            meshes: None,
            count: 0,
            filename: None,
            error: Some(format!("File not found: {}", request.file_path)),
        }));
    }

    let extension = path
        .extension()
        .and_then(|ext| ext.to_str())
        .unwrap_or("")
        .to_lowercase();

    let result = match extension.as_str() {
        "stl" => {
            import_stl(&path)
                .map(|mesh| vec![mesh])
                .map_err(|e| e.to_string())
        }
        "obj" => {
            import_obj(&path)
                .map_err(|e| e.to_string())
        }
        "ply" => {
            import_ply(&path)
                .map(|mesh| vec![mesh])
                .map_err(|e| e.to_string())
        }
        _ => Err(format!("Unsupported file format: {}", extension)),
    };

    match result {
        Ok(mut meshes) => {
            // Apply scale if provided
            if let Some(scale) = request.scale {
                if scale != 1.0 {
                    for mesh in &mut meshes {
                        for vertex in &mut mesh.vertices {
                            *vertex *= scale;
                        }
                    }
                }
            }

            let count = meshes.len();
            let filename = path.file_name().and_then(|n| n.to_str()).map(String::from);

            debug!("Successfully imported {} mesh(es)", count);

            Ok(Json(ImportResponse {
                success: true,
                meshes: Some(meshes),
                count,
                filename,
                error: None,
            }))
        }
        Err(e) => {
            error!("Import failed: {}", e);
            Ok(Json(ImportResponse {
                success: false,
                meshes: None,
                count: 0,
                filename: None,
                error: Some(e),
            }))
        }
    }
}

/// Import STEP (currently unsupported placeholder endpoint)
#[instrument(skip(_state, _request))]
pub async fn import_step(
    State(_state): State<Arc<crate::AppState>>,
    Json(_request): Json<ImportRequest>,
) -> Result<Json<ImportResponse>, StatusCode> {
    Ok(Json(ImportResponse {
        success: false,
        meshes: None,
        count: 0,
        filename: None,
        error: Some("STEP import is not yet implemented".to_string()),
    }))
}

/// Export mesh data
#[instrument(skip(_state))]
pub async fn export_mesh(
    State(_state): State<Arc<crate::AppState>>,
    Json(request): Json<ExportRequest>,
) -> Result<Json<ExportResponse>, StatusCode> {
    debug!("Mesh export requested: {}", request.file_path);

    let path = PathBuf::from(&request.file_path);
    let extension = path
        .extension()
        .and_then(|ext| ext.to_str())
        .unwrap_or("")
        .to_lowercase();

    if request.meshes.is_empty() {
        warn!("No meshes provided for export");
        return Ok(Json(ExportResponse {
            success: false,
            file_path: None,
            error: Some("No meshes provided".to_string()),
        }));
    }

    let result = match extension.as_str() {
        "stl" => {
            if request.meshes.len() > 1 {
                warn!("STL format only supports single mesh, exporting first mesh only");
            }
            export_stl(&request.meshes[0], &path).map_err(|e| e.to_string())
        }
        "obj" => {
            export_obj(&request.meshes, &path).map_err(|e| e.to_string())
        }
        "ply" => {
            if request.meshes.len() > 1 {
                warn!("PLY format only supports single mesh, exporting first mesh only");
            }
            let binary = request.binary.unwrap_or(true);
            export_ply(&request.meshes[0], &path, !binary).map_err(|e| e.to_string())
        }
        _ => Err(format!("Unsupported export format: {}", extension)),
    };

    match result {
        Ok(_) => {
            debug!("Successfully exported to {}", request.file_path);
            Ok(Json(ExportResponse {
                success: true,
                file_path: Some(request.file_path),
                error: None,
            }))
        }
        Err(e) => {
            error!("Export failed: {}", e);
            Ok(Json(ExportResponse {
                success: false,
                file_path: None,
                error: Some(e),
            }))
        }
    }
}

/// Export STEP (currently unsupported placeholder endpoint)
#[instrument(skip(_state, _request))]
pub async fn export_step(
    State(_state): State<Arc<crate::AppState>>,
    Json(_request): Json<ExportRequest>,
) -> Result<Json<ExportResponse>, StatusCode> {
    Ok(Json(ExportResponse {
        success: false,
        file_path: None,
        error: Some("STEP export is not yet implemented".to_string()),
    }))
}

/// Get supported import formats
#[instrument(skip(_state))]
pub async fn get_import_formats(
    State(_state): State<Arc<crate::AppState>>,
) -> Result<Json<Vec<FileFormat>>, StatusCode> {
    debug!("Listing supported import formats");

    let formats = build_formats(IMPORT_FORMATS);

    Ok(Json(formats))
}

/// Get supported export formats
#[instrument(skip(_state))]
pub async fn get_export_formats(
    State(_state): State<Arc<crate::AppState>>,
) -> Result<Json<Vec<FileFormat>>, StatusCode> {
    debug!("Listing supported export formats");

    let formats = build_formats(EXPORT_FORMATS);

    Ok(Json(formats))
}

#[derive(serde::Serialize)]
pub struct FileFormat {
    pub name: String,
    pub extension: String,
    pub description: String,
    pub supported: bool,
}
