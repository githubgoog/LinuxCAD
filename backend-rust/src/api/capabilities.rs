use axum::{extract::State, http::StatusCode, response::Json};
use std::sync::Arc;
use tracing::{debug, instrument};

use super::import_export::{export_format_names, import_format_names};

#[derive(serde::Serialize)]
pub struct CapabilitiesResponse {
    backend: String,
    version: String,
    geometry: GeometryCapabilities,
    sketch: SketchCapabilities,
    import_export: ImportExportCapabilities,
    benchmarks: BenchmarkCapabilities,
}

#[derive(serde::Serialize)]
struct GeometryCapabilities {
    single: bool,
    batch: bool,
    shell: bool,
    boolean: bool,
    fillet: bool,
    chamfer: bool,
}

#[derive(serde::Serialize)]
struct SketchCapabilities {
    solve: bool,
    validate: bool,
}

#[derive(serde::Serialize)]
struct ImportExportCapabilities {
    import_formats: Vec<String>,
    export_formats: Vec<String>,
}

#[derive(serde::Serialize)]
struct BenchmarkCapabilities {
    available: bool,
}

/// Report backend feature capabilities
#[instrument(skip(_state))]
pub async fn get_capabilities(
    State(_state): State<Arc<crate::AppState>>,
) -> Result<Json<CapabilitiesResponse>, StatusCode> {
    debug!("Reporting backend capabilities");

    Ok(Json(CapabilitiesResponse {
        backend: "rust".to_string(),
        version: env!("CARGO_PKG_VERSION").to_string(),
        geometry: GeometryCapabilities {
            single: true,
            batch: true,
            shell: false,
            boolean: false,
            fillet: false,
            chamfer: false,
        },
        sketch: SketchCapabilities {
            solve: true,
            validate: true,
        },
        import_export: ImportExportCapabilities {
            import_formats: import_format_names(),
            export_formats: export_format_names(),
        },
        benchmarks: BenchmarkCapabilities { available: true },
    }))
}
