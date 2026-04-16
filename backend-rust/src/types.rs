use nalgebra::{Point3, Vector3};
use serde::{de, Deserialize, Deserializer, Serialize, Serializer};
use std::collections::HashMap;
use uuid::Uuid;

// ─────────────────────────────────────────────────────────────────────────────
// Core CAD Types (Rust-optimized)
// ─────────────────────────────────────────────────────────────────────────────

#[derive(Debug, Clone, Serialize, Deserialize, Eq, Hash, PartialEq)]
#[serde(rename_all = "camelCase")]
pub enum PrimitiveType {
    Box,
    Cylinder,
    Sphere,
    Cone,
    Torus,
    Wedge,
    Tube,
    Pyramid,
    Ellipsoid,
    Gear,
    Flange,
    ValveWheel,
    Bolt,
    Bearing,
    Pulley,
    Channel,
    Bracket,
    PipeElbow,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub enum AdvancedType {
    Extrude,
    Boolean,
    Mirror,
    Array,
    RadialArray,
    Shell,
    Chamfer,
    Fillet,
    Cut,
    Hole,
    Revolve,
    Sweep,
    Loft,
    Sketch,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(untagged)]
pub enum FeatureType {
    Primitive(PrimitiveType),
    Advanced(AdvancedType),
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub enum LinearUnit {
    Mm,
    Cm,
    M,
    In,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub enum BooleanOp {
    Union,
    Subtract,
    Intersect,
}

// High-performance feature representation
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Feature {
    pub id: String,
    #[serde(rename = "type")]
    pub feature_type: FeatureType,
    pub name: String,
    pub assembly_id: Option<String>,
    pub visible: bool,
    pub color: String,
    pub params: HashMap<String, f64>,
    pub source_ids: Option<Vec<String>>,
    pub baked_geometry: Option<String>,
}

// Optimized mesh data with pre-allocated vectors
#[derive(Debug, Clone)]
pub struct MeshData {
    pub vertices: Vec<f32>, // Flattened xyz coordinates
    pub indices: Vec<u32>,  // Triangle indices
    pub normals: Vec<f32>,  // Vertex normals
    pub edges: Vec<u32>,    // Edge indices for wireframe
}

impl Serialize for MeshData {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        use serde::ser::SerializeMap;

        let mut map = serializer.serialize_map(Some(7))?;
        // Canonical Rust field names.
        map.serialize_entry("vertices", &self.vertices)?;
        map.serialize_entry("indices", &self.indices)?;
        map.serialize_entry("normals", &self.normals)?;
        map.serialize_entry("edges", &self.edges)?;
        // Legacy compatibility field names expected by older clients.
        map.serialize_entry("pos", &self.vertices)?;
        map.serialize_entry("idx", &self.indices)?;
        map.serialize_entry("norm", &self.normals)?;
        map.end()
    }
}

impl<'de> Deserialize<'de> for MeshData {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        #[derive(Deserialize)]
        struct MeshDataCompat {
            vertices: Option<Vec<f32>>,
            indices: Option<Vec<u32>>,
            normals: Option<Vec<f32>>,
            edges: Option<Vec<u32>>,
            pos: Option<Vec<f32>>,
            idx: Option<Vec<u32>>,
            norm: Option<Vec<f32>>,
        }

        let raw = MeshDataCompat::deserialize(deserializer)?;

        let vertices = raw
            .vertices
            .or(raw.pos)
            .ok_or_else(|| de::Error::missing_field("vertices/pos"))?;
        let indices = raw
            .indices
            .or(raw.idx)
            .ok_or_else(|| de::Error::missing_field("indices/idx"))?;
        let normals = raw.normals.or(raw.norm).unwrap_or_default();
        let edges = raw.edges.unwrap_or_default();

        Ok(Self {
            vertices,
            indices,
            normals,
            edges,
        })
    }
}

impl MeshData {
    pub fn new(
        vertices: Vec<Point3<f32>>,
        faces: Vec<[usize; 3]>,
        normals: Vec<Vector3<f32>>,
    ) -> Self {
        let vertices_flat: Vec<f32> = vertices.into_iter().flat_map(|v| [v.x, v.y, v.z]).collect();

        let indices: Vec<u32> = faces
            .into_iter()
            .flat_map(|f| [f[0] as u32, f[1] as u32, f[2] as u32])
            .collect();

        let normals_flat: Vec<f32> = normals.into_iter().flat_map(|v| [v.x, v.y, v.z]).collect();

        // Generate edge indices from faces
        let mut edges = Vec::new();
        for chunk in indices.chunks(3) {
            if chunk.len() == 3 {
                edges.extend_from_slice(&[
                    chunk[0], chunk[1], chunk[1], chunk[2], chunk[2], chunk[0],
                ]);
            }
        }

        Self {
            vertices: vertices_flat,
            indices,
            normals: normals_flat,
            edges,
        }
    }

    pub fn vertex_count(&self) -> usize {
        self.vertices.len() / 3
    }

    pub fn face_count(&self) -> usize {
        self.indices.len() / 3
    }

    pub fn get_vertex(&self, index: usize) -> Option<Point3<f32>> {
        let start = index * 3;
        if start + 2 < self.vertices.len() {
            Some(Point3::new(
                self.vertices[start],
                self.vertices[start + 1],
                self.vertices[start + 2],
            ))
        } else {
            None
        }
    }

    pub fn get_normal(&self, index: usize) -> Option<Vector3<f32>> {
        let start = index * 3;
        if start + 2 < self.normals.len() {
            Some(Vector3::new(
                self.normals[start],
                self.normals[start + 1],
                self.normals[start + 2],
            ))
        } else {
            None
        }
    }

    pub fn validate(&self) -> Result<(), GeometryError> {
        if self.vertices.is_empty() {
            return Err(GeometryError::MeshGenerationFailed(
                "Mesh has no vertices".to_string(),
            ));
        }

        if self.vertices.len() % 3 != 0 {
            return Err(GeometryError::MeshGenerationFailed(
                "Vertices array length must be a multiple of 3".to_string(),
            ));
        }

        if self.indices.len() % 3 != 0 {
            return Err(GeometryError::MeshGenerationFailed(
                "Indices array length must be a multiple of 3".to_string(),
            ));
        }

        if !self.normals.is_empty() && self.normals.len() != self.vertices.len() {
            return Err(GeometryError::MeshGenerationFailed(
                "Normals array length must match vertices array length".to_string(),
            ));
        }

        let vertex_count = self.vertex_count();
        for &idx in &self.indices {
            if idx as usize >= vertex_count {
                return Err(GeometryError::MeshGenerationFailed(format!(
                    "Index {} is out of bounds for {} vertices",
                    idx, vertex_count
                )));
            }
        }

        if self.vertices.iter().any(|value| !value.is_finite()) {
            return Err(GeometryError::MeshGenerationFailed(
                "Vertices contain non-finite values".to_string(),
            ));
        }

        if self.normals.iter().any(|value| !value.is_finite()) {
            return Err(GeometryError::MeshGenerationFailed(
                "Normals contain non-finite values".to_string(),
            ));
        }

        Ok(())
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct GeometryRequest {
    pub features: Vec<Feature>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct GeometryResponse {
    pub feature_id: String,
    pub ok: bool,
    pub mesh: Option<MeshData>,
    pub error: Option<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct BatchGeometryResponse {
    pub results: Vec<GeometryResponse>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ShellRequest {
    pub feature: Feature,
    pub thickness: f64,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct RevolveRequest {
    pub feature: Feature,
    pub axis: String,
    pub side: String,
    pub distance: f64,
    pub angle: f64,
    pub segments: usize,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct SweepRequest {
    pub feature: Feature,
    pub axis: String,
    pub side: String,
    pub distance: f64,
    pub start_angle: f64,
    pub end_angle: f64,
    pub path_rotate: f64,
    pub twist: f64,
    pub segments: usize,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct CutRequest {
    pub feature: Feature,
    pub axis: String,
    pub offset: f64,
    pub keep_side: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct BooleanRequest {
    pub feature_a: Feature,
    pub feature_b: Feature,
    pub op: BooleanOp,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct FilletRequest {
    pub feature: Feature,
    pub radius: f64,
    pub iterations: usize,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct ChamferRequest {
    pub feature: Feature,
    pub amount: f64,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct LoftRequest {
    pub feature_a: Feature,
    pub feature_b: Feature,
    pub sections: usize,
    pub affect_ratio: f64,
}

// ─────────────────────────────────────────────────────────────────────────────
// Sketch Types (High-Performance)
// ─────────────────────────────────────────────────────────────────────────────

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub enum SketchPrimitiveType {
    Line,
    Circle,
    Arc,
    Rectangle,
    Polygon,
    Spline,
    Point,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub enum ConstraintType {
    Coincident,
    Horizontal,
    Vertical,
    Parallel,
    Perpendicular,
    Tangent,
    Equal,
    Symmetric,
    Collinear,
    Concentric,
    Midpoint,
    Distance,
    Radius,
    Diameter,
    Angle,
    Length,
    Fix,
    PatternLinear,
    PatternRadial,
}

// High-performance 2D point for sketching
#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
pub struct SketchPoint {
    pub id: Uuid,
    pub x: f64,
    pub y: f64,
    pub visible: bool,
    pub construction: bool,
}

impl SketchPoint {
    pub fn new(x: f64, y: f64) -> Self {
        Self {
            id: Uuid::new_v4(),
            x,
            y,
            visible: true,
            construction: false,
        }
    }

    pub fn to_nalgebra(&self) -> nalgebra::Point2<f64> {
        nalgebra::Point2::new(self.x, self.y)
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SketchLine {
    pub id: Uuid,
    pub start_point_id: Uuid,
    pub end_point_id: Uuid,
    pub construction: bool,
    pub visible: bool,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SketchCircle {
    pub id: Uuid,
    pub center_point_id: Uuid,
    pub radius: f64,
    pub construction: bool,
    pub visible: bool,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(untagged)]
pub enum SketchEntity {
    Line(SketchLine),
    Circle(SketchCircle),
    // Add other entity types as needed
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct GeometricConstraint {
    pub id: Uuid,
    pub constraint_type: ConstraintType,
    pub entity_ids: Vec<Uuid>,
    pub point_ids: Option<Vec<Uuid>>,
    pub value: Option<f64>,
    pub enabled: bool,
    pub satisfied: bool,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Sketch {
    pub id: Uuid,
    pub name: String,
    pub points: Vec<SketchPoint>,
    pub entities: Vec<SketchEntity>,
    pub constraints: Vec<GeometricConstraint>,
    pub visible: bool,
    pub active: bool,
    pub fully_constrained: bool,
}

// Constraint solver types
#[derive(Debug, Clone)]
pub struct Variable {
    pub id: String,
    pub value: f64,
    pub min_value: Option<f64>,
    pub max_value: Option<f64>,
    pub fixed: bool,
}

#[derive(Debug, Clone)]
pub struct SolverConstraint {
    pub id: String,
    pub constraint_type: String,
    pub variables: Vec<String>,
    pub target_value: Option<f64>,
    pub weight: f64,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SolverResult {
    pub success: bool,
    pub iterations: u32,
    pub residual: f64,
    pub variables: HashMap<String, f64>,
    pub constraint_errors: HashMap<String, f64>,
}

// ─────────────────────────────────────────────────────────────────────────────
// Material System Types
// ─────────────────────────────────────────────────────────────────────────────

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct MaterialProperties {
    pub density: f64,         // kg/m³
    pub elastic_modulus: f64, // Pa
    pub poissons_ratio: f64,
    pub thermal_expansion: f64,         // 1/K
    pub thermal_conductivity: f64,      // W/(m·K)
    pub specific_heat: f64,             // J/(kg·K)
    pub yield_strength: Option<f64>,    // Pa
    pub ultimate_strength: Option<f64>, // Pa
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct MaterialAppearance {
    pub base_color: [f32; 4], // RGBA
    pub metallic: f32,
    pub roughness: f32,
    pub normal_scale: f32,
    pub emissive: [f32; 3], // RGB
    pub transparency: f32,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Material {
    pub id: String,
    pub name: String,
    pub category: String, // "Steel", "Aluminum", "Plastic", etc.
    pub properties: MaterialProperties,
    pub appearance: MaterialAppearance,
    pub custom: bool,
}

// ─────────────────────────────────────────────────────────────────────────────
// Error Types
// ─────────────────────────────────────────────────────────────────────────────

#[derive(Debug, thiserror::Error)]
pub enum GeometryError {
    #[error("Invalid feature parameters: {0}")]
    InvalidParameters(String),

    #[error("Mesh generation failed: {0}")]
    MeshGenerationFailed(String),

    #[error("Boolean operation failed: {0}")]
    BooleanOperationFailed(String),

    #[error("File operation failed: {0}")]
    FileOperationFailed(String),
}

#[derive(Debug, thiserror::Error)]
pub enum ConstraintError {
    #[error("Constraint solving failed: {0}")]
    SolvingFailed(String),

    #[error("Invalid constraint: {0}")]
    InvalidConstraint(String),

    #[error("Over-constrained system")]
    OverConstrained,

    #[error("Under-constrained system")]
    UnderConstrained,
}
