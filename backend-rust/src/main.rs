use axum::{
    extract::{Query, State},
    http::StatusCode,
    response::Json,
    routing::{get, post},
    Router,
};
use serde::{Deserialize, Serialize};
use std::{collections::HashMap, io, sync::Arc};
use tower_http::cors::CorsLayer;
use tracing::{info, instrument};
use sysinfo::System;
use tokio::io::{AsyncReadExt, AsyncWriteExt};

mod types;
mod geometry;
mod sketch;
mod constraint;
mod import_export;
mod api;
mod benchmarks;

use types::*;

#[derive(Debug, Clone)]
pub struct AppState {
    // Application-wide state and caches
}

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    // Initialize tracing
    tracing_subscriber::fmt::init();

    info!("Starting LinuxCAD Rust Backend");

    let state = AppState {};

    // Build our application with routes
    let app = Router::new()
        // Health check
        .route("/api/health", get(health_check))
        .route("/api/capabilities", get(api::capabilities::get_capabilities))

        // Geometry operations
        .route("/api/geometry/single", post(api::geometry::compute_single))
        .route("/api/geometry/batch", post(api::geometry::compute_batch))
        .route("/api/operations/shell", post(api::geometry::shell_operation))

        // Constraint solving
        .route("/api/sketch/solve", post(api::sketch::solve_constraints))
        .route("/api/sketch/validate", post(api::sketch::validate_sketch))

        // Import/Export
        .route("/api/import/mesh", post(api::import_export::import_mesh))
        .route("/api/import/step", post(api::import_export::import_step))
        .route("/api/export/mesh", post(api::import_export::export_mesh))
        .route("/api/export/step", post(api::import_export::export_step))
        .route("/api/formats/import", get(api::import_export::get_import_formats))
        .route("/api/formats/export", get(api::import_export::get_export_formats))

        // Advanced operations
        .route("/api/boolean", post(api::geometry::boolean_operation))
        .route("/api/fillet", post(api::geometry::fillet_operation))
        .route("/api/chamfer", post(api::geometry::chamfer_operation))

        // Performance and benchmarks
        .route("/api/benchmark/run", post(run_benchmark_suite))
        .route("/api/system/status", get(get_system_status))

        .with_state(Arc::new(state))
        .layer(CorsLayer::permissive());

    let port = std::env::var("PORT")
        .unwrap_or_else(|_| "8000".to_string())
        .parse::<u16>()
        .expect("PORT must be a valid number");

    let bind_address = format!("0.0.0.0:{}", port);
    let listener = match tokio::net::TcpListener::bind(&bind_address).await {
        Ok(listener) => listener,
        Err(error) if error.kind() == io::ErrorKind::AddrInUse => {
            if backend_is_healthy(port).await {
                info!("LinuxCAD Rust backend already running on port {}", port);
                return Ok(());
            }

            return Err(error.into());
        }
        Err(error) => return Err(error.into()),
    };

    info!("🚀 LinuxCAD Rust backend listening on port {}", port);

    axum::serve(listener, app)
        .await
        .expect("Failed to start server");

    Ok(())
}

#[instrument]
async fn health_check() -> Json<HashMap<String, String>> {
    let mut response = HashMap::new();
    response.insert("status".to_string(), "ok".to_string());
    response.insert("backend".to_string(), "rust".to_string());
    response.insert("version".to_string(), env!("CARGO_PKG_VERSION").to_string());
    Json(response)
}

#[instrument]
async fn run_benchmark_suite() -> Result<Json<benchmarks::BenchmarkSuite>, StatusCode> {
    info!("Starting benchmark suite execution");

    let suite = benchmarks::run_full_benchmark_suite().await;

    info!("Benchmark suite completed with {} results", suite.results.len());
    Ok(Json(suite))
}

#[instrument]
async fn get_system_status() -> Json<HashMap<String, serde_json::Value>> {
    let mut response = HashMap::new();

    // Get system information
    let mut sys = sysinfo::System::new_all();
    sys.refresh_all();

    response.insert("backend".to_string(), serde_json::Value::String("rust".to_string()));
    response.insert("version".to_string(), serde_json::Value::String(env!("CARGO_PKG_VERSION").to_string()));
    response.insert("status".to_string(), serde_json::Value::String("running".to_string()));
    response.insert("uptime_ms".to_string(), serde_json::Value::Number(
        serde_json::Number::from(std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .unwrap()
            .as_millis() as u64)
    ));

    // System metrics
    response.insert("cpu_cores".to_string(), serde_json::Value::Number(
        serde_json::Number::from(num_cpus::get())
    ));
    response.insert("memory_total_gb".to_string(), serde_json::Value::Number(
        serde_json::Number::from_f64(sys.total_memory() as f64 / 1024.0 / 1024.0 / 1024.0).unwrap()
    ));
    response.insert("memory_used_gb".to_string(), serde_json::Value::Number(
        serde_json::Number::from_f64(sys.used_memory() as f64 / 1024.0 / 1024.0 / 1024.0).unwrap()
    ));

    Json(response)
}

async fn backend_is_healthy(port: u16) -> bool {
    let Ok(mut stream) = tokio::net::TcpStream::connect(("127.0.0.1", port)).await else {
        return false;
    };

    let request = format!(
        "GET /api/health HTTP/1.1\r\nHost: 127.0.0.1:{}\r\nConnection: close\r\n\r\n",
        port
    );

    if stream.write_all(request.as_bytes()).await.is_err() {
        return false;
    }

    let mut response = Vec::new();
    if stream.read_to_end(&mut response).await.is_err() {
        return false;
    }

    let body = String::from_utf8_lossy(&response);
    body.contains("200 OK") && body.contains("\"backend\":\"rust\"")
}
