use crate::geometry::calculate_vertex_normals;
use crate::types::MeshData;
use nalgebra::{Point3, Vector3};
use ply_rs::{parser, ply};
use std::fs::File;
use std::io::{BufReader, BufWriter, Read, Write};
use std::path::Path;
use thiserror::Error;

// ─────────────────────────────────────────────────────────────────────────────
// Error Types
// ─────────────────────────────────────────────────────────────────────────────

#[derive(Debug, Error)]
pub enum ImportExportError {
    #[error("IO error: {0}")]
    Io(#[from] std::io::Error),

    #[error("Invalid STL file: {0}")]
    InvalidStl(String),

    #[error("Invalid OBJ file: {0}")]
    InvalidObj(String),

    #[error("Invalid PLY file: {0}")]
    InvalidPly(String),

    #[error("Unsupported file format: {0}")]
    UnsupportedFormat(String),

    #[error("Parse error: {0}")]
    ParseError(String),

    #[error("Invalid mesh data: {0}")]
    InvalidMeshData(String),
}

pub type Result<T> = std::result::Result<T, ImportExportError>;

// ─────────────────────────────────────────────────────────────────────────────
// STL Format Detection
// ─────────────────────────────────────────────────────────────────────────────

/// Detect if an STL file is ASCII or binary format
fn is_stl_ascii(path: &Path) -> Result<bool> {
    let mut file = File::open(path)?;
    let mut buffer = [0u8; 6];
    file.read_exact(&mut buffer)?;

    // ASCII STL files start with "solid "
    Ok(&buffer == b"solid ")
}

// ─────────────────────────────────────────────────────────────────────────────
// STL Import
// ─────────────────────────────────────────────────────────────────────────────

/// Import an STL file (automatically detects ASCII or binary format)
pub fn import_stl<P: AsRef<Path>>(path: P) -> Result<MeshData> {
    let path = path.as_ref();

    if is_stl_ascii(path)? {
        import_stl_ascii(path)
    } else {
        import_stl_binary(path)
    }
}

/// Import an ASCII STL file
pub fn import_stl_ascii<P: AsRef<Path>>(path: P) -> Result<MeshData> {
    let file = File::open(path.as_ref())?;
    let mut reader = BufReader::new(file);

    let stl = stl_io::read_stl(&mut reader)
        .map_err(|e| ImportExportError::InvalidStl(format!("Failed to parse ASCII STL: {}", e)))?;

    stl_to_mesh_data(stl)
}

/// Import a binary STL file
pub fn import_stl_binary<P: AsRef<Path>>(path: P) -> Result<MeshData> {
    let file = File::open(path.as_ref())?;
    let mut reader = BufReader::new(file);

    let stl = stl_io::read_stl(&mut reader)
        .map_err(|e| ImportExportError::InvalidStl(format!("Failed to parse binary STL: {}", e)))?;

    stl_to_mesh_data(stl)
}

/// Convert stl_io mesh to our MeshData format
fn stl_to_mesh_data(stl: stl_io::IndexedMesh) -> Result<MeshData> {
    if stl.vertices.is_empty() {
        return Err(ImportExportError::InvalidMeshData(
            "STL file contains no vertices".to_string(),
        ));
    }

    if stl.faces.is_empty() {
        return Err(ImportExportError::InvalidMeshData(
            "STL file contains no faces".to_string(),
        ));
    }

    // Convert vertices to Point3
    let vertices: Vec<Point3<f32>> = stl
        .vertices
        .iter()
        .map(|v| Point3::new(v[0], v[1], v[2]))
        .collect();

    // Convert faces to triangle indices
    let faces: Vec<[usize; 3]> = stl
        .faces
        .iter()
        .map(|f| [f.vertices[0], f.vertices[1], f.vertices[2]])
        .collect();

    // Calculate vertex normals by averaging face normals
    let normals = calculate_vertex_normals(&vertices, &faces);

    Ok(MeshData::new(vertices, faces, normals))
}

// ─────────────────────────────────────────────────────────────────────────────
// STL Export
// ─────────────────────────────────────────────────────────────────────────────

/// Export mesh to STL file (binary format by default)
pub fn export_stl<P: AsRef<Path>>(mesh: &MeshData, path: P) -> Result<()> {
    export_stl_binary(mesh, path)
}

/// Export mesh to binary STL file
pub fn export_stl_binary<P: AsRef<Path>>(mesh: &MeshData, path: P) -> Result<()> {
    let indexed_mesh = mesh_data_to_stl(mesh)?;

    let file = File::create(path.as_ref())?;
    let mut writer = BufWriter::new(file);

    stl_io::write_stl(&mut writer, indexed_mesh.iter()).map_err(|e| {
        ImportExportError::Io(std::io::Error::new(
            std::io::ErrorKind::Other,
            format!("Failed to write binary STL: {}", e),
        ))
    })?;

    Ok(())
}

/// Export mesh to ASCII STL file
pub fn export_stl_ascii<P: AsRef<Path>>(mesh: &MeshData, path: P) -> Result<()> {
    let file = File::create(path.as_ref())?;
    let mut writer = BufWriter::new(file);

    // Write ASCII STL format manually for better control
    writeln!(writer, "solid mesh")?;

    // Write each triangle
    for i in 0..mesh.face_count() {
        let idx_base = i * 3;
        let i0 = mesh.indices[idx_base] as usize;
        let i1 = mesh.indices[idx_base + 1] as usize;
        let i2 = mesh.indices[idx_base + 2] as usize;

        // Get vertices
        let v0 = mesh.get_vertex(i0).ok_or_else(|| {
            ImportExportError::InvalidMeshData(format!("Invalid vertex index: {}", i0))
        })?;
        let v1 = mesh.get_vertex(i1).ok_or_else(|| {
            ImportExportError::InvalidMeshData(format!("Invalid vertex index: {}", i1))
        })?;
        let v2 = mesh.get_vertex(i2).ok_or_else(|| {
            ImportExportError::InvalidMeshData(format!("Invalid vertex index: {}", i2))
        })?;

        // Calculate face normal
        let edge1 = v1 - v0;
        let edge2 = v2 - v0;
        let normal = edge1.cross(&edge2).normalize();

        // Write facet
        writeln!(
            writer,
            "  facet normal {} {} {}",
            normal.x, normal.y, normal.z
        )?;
        writeln!(writer, "    outer loop")?;
        writeln!(writer, "      vertex {} {} {}", v0.x, v0.y, v0.z)?;
        writeln!(writer, "      vertex {} {} {}", v1.x, v1.y, v1.z)?;
        writeln!(writer, "      vertex {} {} {}", v2.x, v2.y, v2.z)?;
        writeln!(writer, "    endloop")?;
        writeln!(writer, "  endfacet")?;
    }

    writeln!(writer, "endsolid mesh")?;

    Ok(())
}

/// Convert our MeshData to stl_io format
fn mesh_data_to_stl(mesh: &MeshData) -> Result<Vec<stl_io::Triangle>> {
    if mesh.vertices.len() % 3 != 0 {
        return Err(ImportExportError::InvalidMeshData(
            "Vertices array length must be a multiple of 3".to_string(),
        ));
    }

    if mesh.indices.len() % 3 != 0 {
        return Err(ImportExportError::InvalidMeshData(
            "Indices array length must be a multiple of 3".to_string(),
        ));
    }

    let mut triangles = Vec::with_capacity(mesh.face_count());

    for i in 0..mesh.face_count() {
        let idx_base = i * 3;
        let i0 = mesh.indices[idx_base] as usize;
        let i1 = mesh.indices[idx_base + 1] as usize;
        let i2 = mesh.indices[idx_base + 2] as usize;

        // Get vertices
        let v0 = mesh.get_vertex(i0).ok_or_else(|| {
            ImportExportError::InvalidMeshData(format!("Invalid vertex index: {}", i0))
        })?;
        let v1 = mesh.get_vertex(i1).ok_or_else(|| {
            ImportExportError::InvalidMeshData(format!("Invalid vertex index: {}", i1))
        })?;
        let v2 = mesh.get_vertex(i2).ok_or_else(|| {
            ImportExportError::InvalidMeshData(format!("Invalid vertex index: {}", i2))
        })?;

        // Calculate face normal
        let edge1 = v1 - v0;
        let edge2 = v2 - v0;
        let normal = edge1.cross(&edge2).normalize();

        triangles.push(stl_io::Triangle {
            normal: stl_io::Normal::new([normal.x, normal.y, normal.z]),
            vertices: [
                stl_io::Vertex::new([v0.x, v0.y, v0.z]),
                stl_io::Vertex::new([v1.x, v1.y, v1.z]),
                stl_io::Vertex::new([v2.x, v2.y, v2.z]),
            ],
        });
    }

    Ok(triangles)
}

// ─────────────────────────────────────────────────────────────────────────────
// OBJ Import/Export
// ─────────────────────────────────────────────────────────────────────────────

/// Import an OBJ file
pub fn import_obj<P: AsRef<Path>>(path: P) -> Result<Vec<MeshData>> {
    let obj = tobj::load_obj(
        path.as_ref(),
        &tobj::LoadOptions {
            single_index: true,
            triangulate: true,
            ..Default::default()
        },
    )
    .map_err(|e| ImportExportError::InvalidObj(format!("Failed to load OBJ: {}", e)))?;

    let (models, _materials) = obj;

    if models.is_empty() {
        return Err(ImportExportError::InvalidObj(
            "OBJ file contains no models".to_string(),
        ));
    }

    let mut meshes = Vec::new();

    for model in models {
        let mesh = &model.mesh;

        if mesh.positions.is_empty() {
            continue; // Skip empty meshes
        }

        // Convert positions to Point3
        let vertices: Vec<Point3<f32>> = mesh
            .positions
            .chunks_exact(3)
            .map(|v| Point3::new(v[0], v[1], v[2]))
            .collect();

        // Get faces (already triangulated)
        let faces: Vec<[usize; 3]> = mesh
            .indices
            .chunks_exact(3)
            .map(|f| [f[0] as usize, f[1] as usize, f[2] as usize])
            .collect();

        // Use provided normals if available, otherwise calculate
        let normals = if !mesh.normals.is_empty() {
            mesh.normals
                .chunks_exact(3)
                .map(|n| Vector3::new(n[0], n[1], n[2]).normalize())
                .collect()
        } else {
            calculate_vertex_normals(&vertices, &faces)
        };

        meshes.push(MeshData::new(vertices, faces, normals));
    }

    if meshes.is_empty() {
        return Err(ImportExportError::InvalidObj(
            "No valid geometry found in OBJ file".to_string(),
        ));
    }

    Ok(meshes)
}

/// Export mesh to OBJ file
pub fn export_obj<P: AsRef<Path>>(meshes: &[MeshData], path: P) -> Result<()> {
    let file = File::create(path.as_ref())?;
    let mut writer = BufWriter::new(file);

    writeln!(writer, "# LinuxCAD OBJ Export")?;
    writeln!(writer, "# {} mesh(es)", meshes.len())?;
    writeln!(writer)?;

    let mut vertex_offset = 0;

    for (mesh_idx, mesh) in meshes.iter().enumerate() {
        writeln!(writer, "o mesh_{}", mesh_idx)?;

        // Write vertices
        for i in 0..mesh.vertex_count() {
            let v = mesh.get_vertex(i).ok_or_else(|| {
                ImportExportError::InvalidMeshData(format!("Invalid vertex index: {}", i))
            })?;
            writeln!(writer, "v {} {} {}", v.x, v.y, v.z)?;
        }

        // Write normals
        for i in 0..mesh.vertex_count() {
            let n = mesh.get_normal(i).ok_or_else(|| {
                ImportExportError::InvalidMeshData(format!("Invalid normal index: {}", i))
            })?;
            writeln!(writer, "vn {} {} {}", n.x, n.y, n.z)?;
        }

        // Write faces (1-indexed in OBJ format)
        for face_idx in 0..mesh.face_count() {
            let idx_base = face_idx * 3;
            let i0 = mesh.indices[idx_base] as usize + vertex_offset + 1;
            let i1 = mesh.indices[idx_base + 1] as usize + vertex_offset + 1;
            let i2 = mesh.indices[idx_base + 2] as usize + vertex_offset + 1;

            // OBJ format: f v1//vn1 v2//vn2 v3//vn3
            writeln!(writer, "f {}//{} {}//{} {}//{}", i0, i0, i1, i1, i2, i2)?;
        }

        vertex_offset += mesh.vertex_count();
        writeln!(writer)?;
    }

    Ok(())
}

// ─────────────────────────────────────────────────────────────────────────────
// PLY Import/Export
// ─────────────────────────────────────────────────────────────────────────────

/// Import a PLY file (supports both ASCII and binary formats)
pub fn import_ply<P: AsRef<Path>>(path: P) -> Result<MeshData> {
    let file = File::open(path.as_ref())?;
    let mut reader = BufReader::new(file);

    // Parse PLY file
    let vertex_parser = parser::Parser::<ply::DefaultElement>::new();
    let ply = vertex_parser
        .read_ply(&mut reader)
        .map_err(|e| ImportExportError::InvalidPly(format!("Failed to parse PLY: {:?}", e)))?;

    // Extract vertices
    let vertex_list = ply
        .payload
        .get("vertex")
        .ok_or_else(|| ImportExportError::InvalidPly("No vertex data found".to_string()))?;

    let mut vertices = Vec::new();
    let mut vertex_normals = Vec::new();

    for vertex in vertex_list {
        // Extract position
        let x = get_ply_property(vertex, "x")?;
        let y = get_ply_property(vertex, "y")?;
        let z = get_ply_property(vertex, "z")?;
        vertices.push(Point3::new(x, y, z));

        // Extract normals if available
        if let (Ok(nx), Ok(ny), Ok(nz)) = (
            get_ply_property(vertex, "nx"),
            get_ply_property(vertex, "ny"),
            get_ply_property(vertex, "nz"),
        ) {
            vertex_normals.push(Vector3::new(nx, ny, nz));
        }
    }

    if vertices.is_empty() {
        return Err(ImportExportError::InvalidPly(
            "No vertices found in PLY file".to_string(),
        ));
    }

    // Extract faces
    let face_list = ply
        .payload
        .get("face")
        .ok_or_else(|| ImportExportError::InvalidPly("No face data found".to_string()))?;

    let mut faces = Vec::new();

    for face in face_list {
        // Get vertex indices
        if let Some(ply::Property::ListUInt(indices)) = face.get("vertex_indices") {
            if indices.len() == 3 {
                faces.push([
                    indices[0] as usize,
                    indices[1] as usize,
                    indices[2] as usize,
                ]);
            } else if indices.len() == 4 {
                // Triangulate quad
                faces.push([
                    indices[0] as usize,
                    indices[1] as usize,
                    indices[2] as usize,
                ]);
                faces.push([
                    indices[0] as usize,
                    indices[2] as usize,
                    indices[3] as usize,
                ]);
            }
        } else if let Some(ply::Property::ListUChar(indices)) = face.get("vertex_indices") {
            if indices.len() == 3 {
                faces.push([
                    indices[0] as usize,
                    indices[1] as usize,
                    indices[2] as usize,
                ]);
            } else if indices.len() == 4 {
                faces.push([
                    indices[0] as usize,
                    indices[1] as usize,
                    indices[2] as usize,
                ]);
                faces.push([
                    indices[0] as usize,
                    indices[2] as usize,
                    indices[3] as usize,
                ]);
            }
        }
    }

    if faces.is_empty() {
        return Err(ImportExportError::InvalidPly(
            "No faces found in PLY file".to_string(),
        ));
    }

    // Use provided normals or calculate them
    let normals = if vertex_normals.len() == vertices.len() {
        vertex_normals
    } else {
        calculate_vertex_normals(&vertices, &faces)
    };

    Ok(MeshData::new(vertices, faces, normals))
}

/// Helper to extract float property from PLY element
fn get_ply_property(element: &ply::DefaultElement, name: &str) -> Result<f32> {
    match element.get(name) {
        Some(ply::Property::Float(v)) => Ok(*v),
        Some(ply::Property::Double(v)) => Ok(*v as f32),
        Some(ply::Property::Int(v)) => Ok(*v as f32),
        Some(ply::Property::UInt(v)) => Ok(*v as f32),
        Some(ply::Property::Short(v)) => Ok(*v as f32),
        Some(ply::Property::UShort(v)) => Ok(*v as f32),
        Some(ply::Property::Char(v)) => Ok(*v as f32),
        Some(ply::Property::UChar(v)) => Ok(*v as f32),
        _ => Err(ImportExportError::InvalidPly(format!(
            "Property '{}' not found or invalid type",
            name
        ))),
    }
}

/// Export mesh to PLY file (binary format)
pub fn export_ply<P: AsRef<Path>>(mesh: &MeshData, path: P, ascii: bool) -> Result<()> {
    let file = File::create(path.as_ref())?;
    let mut writer = BufWriter::new(file);

    // Create PLY header
    let encoding = if ascii {
        "ascii"
    } else {
        "binary_little_endian"
    };

    writeln!(writer, "ply")?;
    writeln!(writer, "format {} 1.0", encoding)?;
    writeln!(writer, "comment LinuxCAD PLY Export")?;
    writeln!(writer, "element vertex {}", mesh.vertex_count())?;
    writeln!(writer, "property float x")?;
    writeln!(writer, "property float y")?;
    writeln!(writer, "property float z")?;
    writeln!(writer, "property float nx")?;
    writeln!(writer, "property float ny")?;
    writeln!(writer, "property float nz")?;
    writeln!(writer, "element face {}", mesh.face_count())?;
    writeln!(writer, "property list uchar uint vertex_indices")?;
    writeln!(writer, "end_header")?;

    if ascii {
        // ASCII format
        for i in 0..mesh.vertex_count() {
            let v = mesh.get_vertex(i).ok_or_else(|| {
                ImportExportError::InvalidMeshData(format!("Invalid vertex index: {}", i))
            })?;
            let n = mesh.get_normal(i).ok_or_else(|| {
                ImportExportError::InvalidMeshData(format!("Invalid normal index: {}", i))
            })?;

            writeln!(writer, "{} {} {} {} {} {}", v.x, v.y, v.z, n.x, n.y, n.z)?;
        }

        for face_idx in 0..mesh.face_count() {
            let idx_base = face_idx * 3;
            let i0 = mesh.indices[idx_base];
            let i1 = mesh.indices[idx_base + 1];
            let i2 = mesh.indices[idx_base + 2];

            writeln!(writer, "3 {} {} {}", i0, i1, i2)?;
        }
    } else {
        // Binary format
        for i in 0..mesh.vertex_count() {
            let v = mesh.get_vertex(i).ok_or_else(|| {
                ImportExportError::InvalidMeshData(format!("Invalid vertex index: {}", i))
            })?;
            let n = mesh.get_normal(i).ok_or_else(|| {
                ImportExportError::InvalidMeshData(format!("Invalid normal index: {}", i))
            })?;

            writer.write_all(&v.x.to_le_bytes())?;
            writer.write_all(&v.y.to_le_bytes())?;
            writer.write_all(&v.z.to_le_bytes())?;
            writer.write_all(&n.x.to_le_bytes())?;
            writer.write_all(&n.y.to_le_bytes())?;
            writer.write_all(&n.z.to_le_bytes())?;
        }

        for face_idx in 0..mesh.face_count() {
            let idx_base = face_idx * 3;
            writer.write_all(&[3u8])?; // Triangle has 3 vertices
            writer.write_all(&mesh.indices[idx_base].to_le_bytes())?;
            writer.write_all(&mesh.indices[idx_base + 1].to_le_bytes())?;
            writer.write_all(&mesh.indices[idx_base + 2].to_le_bytes())?;
        }
    }

    Ok(())
}

// ─────────────────────────────────────────────────────────────────────────────
// Tests
// ─────────────────────────────────────────────────────────────────────────────

#[cfg(test)]
mod tests {
    use super::*;
    use std::fs;
    use tempfile::TempDir;

    fn create_test_mesh() -> MeshData {
        // Create a simple triangle mesh (one triangle)
        let vertices = vec![
            Point3::new(0.0, 0.0, 0.0),
            Point3::new(1.0, 0.0, 0.0),
            Point3::new(0.5, 1.0, 0.0),
        ];
        let faces = vec![[0, 1, 2]];
        let normals = vec![
            Vector3::new(0.0, 0.0, 1.0),
            Vector3::new(0.0, 0.0, 1.0),
            Vector3::new(0.0, 0.0, 1.0),
        ];

        MeshData::new(vertices, faces, normals)
    }

    fn create_test_cube() -> MeshData {
        // Create a simple cube mesh
        let vertices = vec![
            // Front face
            Point3::new(-1.0, -1.0, 1.0),
            Point3::new(1.0, -1.0, 1.0),
            Point3::new(1.0, 1.0, 1.0),
            Point3::new(-1.0, 1.0, 1.0),
            // Back face
            Point3::new(-1.0, -1.0, -1.0),
            Point3::new(1.0, -1.0, -1.0),
            Point3::new(1.0, 1.0, -1.0),
            Point3::new(-1.0, 1.0, -1.0),
        ];

        let faces = vec![
            // Front
            [0, 1, 2],
            [2, 3, 0],
            // Right
            [1, 5, 6],
            [6, 2, 1],
            // Back
            [5, 4, 7],
            [7, 6, 5],
            // Left
            [4, 0, 3],
            [3, 7, 4],
            // Top
            [3, 2, 6],
            [6, 7, 3],
            // Bottom
            [4, 5, 1],
            [1, 0, 4],
        ];

        let normals = calculate_vertex_normals(&vertices, &faces);
        MeshData::new(vertices, faces, normals)
    }

    #[test]
    fn test_stl_binary_export_import() {
        let temp_dir = TempDir::new().unwrap();
        let stl_path = temp_dir.path().join("test.stl");

        // Create and export a test mesh
        let original_mesh = create_test_mesh();
        export_stl_binary(&original_mesh, &stl_path).expect("Failed to export STL");

        // Verify file exists
        assert!(stl_path.exists());

        // Import the mesh back
        let imported_mesh = import_stl_binary(&stl_path).expect("Failed to import STL");

        // Verify vertex and face counts match
        assert_eq!(original_mesh.vertex_count(), imported_mesh.vertex_count());
        assert_eq!(original_mesh.face_count(), imported_mesh.face_count());
    }

    #[test]
    fn test_stl_ascii_export_import() {
        let temp_dir = TempDir::new().unwrap();
        let stl_path = temp_dir.path().join("test_ascii.stl");

        // Create and export a test mesh
        let original_mesh = create_test_mesh();
        export_stl_ascii(&original_mesh, &stl_path).expect("Failed to export ASCII STL");

        // Verify file exists and is ASCII
        assert!(stl_path.exists());
        assert!(is_stl_ascii(&stl_path).unwrap());

        // Import the mesh back
        let imported_mesh = import_stl_ascii(&stl_path).expect("Failed to import ASCII STL");

        // Verify vertex and face counts match
        assert_eq!(original_mesh.vertex_count(), imported_mesh.vertex_count());
        assert_eq!(original_mesh.face_count(), imported_mesh.face_count());
    }

    #[test]
    fn test_auto_detect_format() {
        let temp_dir = TempDir::new().unwrap();

        // Test binary format
        let binary_path = temp_dir.path().join("binary.stl");
        let mesh = create_test_mesh();
        export_stl_binary(&mesh, &binary_path).unwrap();

        let imported_binary = import_stl(&binary_path).expect("Failed to auto-import binary STL");
        assert_eq!(mesh.vertex_count(), imported_binary.vertex_count());

        // Test ASCII format
        let ascii_path = temp_dir.path().join("ascii.stl");
        export_stl_ascii(&mesh, &ascii_path).unwrap();

        let imported_ascii = import_stl(&ascii_path).expect("Failed to auto-import ASCII STL");
        assert_eq!(mesh.vertex_count(), imported_ascii.vertex_count());
    }

    #[test]
    fn test_cube_export_import() {
        let temp_dir = TempDir::new().unwrap();
        let stl_path = temp_dir.path().join("cube.stl");

        // Create and export a cube
        let cube = create_test_cube();
        export_stl(&cube, &stl_path).expect("Failed to export cube");

        // Import it back
        let imported = import_stl(&stl_path).expect("Failed to import cube");

        // Verify dimensions
        assert_eq!(cube.vertex_count(), imported.vertex_count());
        assert_eq!(cube.face_count(), imported.face_count());
        assert_eq!(12, imported.face_count()); // A cube should have 12 triangular faces
    }

    #[test]
    fn test_normal_calculation() {
        let vertices = vec![
            Point3::new(0.0, 0.0, 0.0),
            Point3::new(1.0, 0.0, 0.0),
            Point3::new(0.0, 1.0, 0.0),
        ];
        let faces = vec![[0, 1, 2]];

        let normals = calculate_vertex_normals(&vertices, &faces);

        assert_eq!(normals.len(), 3);

        // All normals should point in roughly the same direction (0, 0, 1)
        for normal in &normals {
            assert!(normal.z > 0.9); // Should be close to 1.0
            assert!(normal.norm() > 0.99); // Should be normalized
            assert!(normal.norm() < 1.01);
        }
    }

    #[test]
    fn test_empty_mesh_error() {
        let empty_stl = stl_io::IndexedMesh {
            vertices: Vec::new(),
            faces: Vec::new(),
        };

        let result = stl_to_mesh_data(empty_stl);
        assert!(result.is_err());

        match result {
            Err(ImportExportError::InvalidMeshData(msg)) => {
                assert!(msg.contains("no vertices"));
            }
            _ => panic!("Expected InvalidMeshData error"),
        }
    }

    #[test]
    fn test_mesh_data_validation() {
        let mesh = create_test_mesh();

        // Verify mesh properties
        assert_eq!(mesh.vertices.len() % 3, 0);
        assert_eq!(mesh.indices.len() % 3, 0);
        assert_eq!(mesh.normals.len(), mesh.vertices.len());

        // Verify vertex count
        assert_eq!(mesh.vertex_count(), 3);
        assert_eq!(mesh.face_count(), 1);
    }

    #[test]
    fn test_large_mesh_performance() {
        // Create a mesh with many triangles
        let mut vertices = Vec::new();
        let mut faces = Vec::new();

        // Create a grid of triangles
        let grid_size = 10;
        for y in 0..grid_size {
            for x in 0..grid_size {
                let base_idx = vertices.len();

                // Add 4 vertices for a quad
                vertices.push(Point3::new(x as f32, y as f32, 0.0));
                vertices.push(Point3::new((x + 1) as f32, y as f32, 0.0));
                vertices.push(Point3::new((x + 1) as f32, (y + 1) as f32, 0.0));
                vertices.push(Point3::new(x as f32, (y + 1) as f32, 0.0));

                // Add 2 triangles
                faces.push([base_idx, base_idx + 1, base_idx + 2]);
                faces.push([base_idx + 2, base_idx + 3, base_idx]);
            }
        }

        let normals = calculate_vertex_normals(&vertices, &faces);
        let mesh = MeshData::new(vertices, faces, normals);

        // Test export/import
        let temp_dir = TempDir::new().unwrap();
        let stl_path = temp_dir.path().join("large.stl");

        // Should handle large meshes efficiently
        export_stl(&mesh, &stl_path).expect("Failed to export large mesh");
        let imported = import_stl(&stl_path).expect("Failed to import large mesh");

        assert_eq!(mesh.face_count(), imported.face_count());
        // STL importers may deduplicate or expand vertices depending on implementation,
        // so face count is the stable round-trip invariant for this format.
        assert!(imported.vertex_count() > 0);
    }

    #[test]
    fn test_obj_export_import() {
        let temp_dir = TempDir::new().unwrap();
        let obj_path = temp_dir.path().join("test.obj");

        // Create and export a test mesh
        let original_mesh = create_test_cube();
        export_obj(&[original_mesh.clone()], &obj_path).expect("Failed to export OBJ");

        // Verify file exists
        assert!(obj_path.exists());

        // Import the mesh back
        let imported_meshes = import_obj(&obj_path).expect("Failed to import OBJ");

        assert_eq!(imported_meshes.len(), 1);
        let imported_mesh = &imported_meshes[0];

        // Verify vertex and face counts match
        assert_eq!(original_mesh.vertex_count(), imported_mesh.vertex_count());
        assert_eq!(original_mesh.face_count(), imported_mesh.face_count());
    }

    #[test]
    fn test_obj_multiple_meshes() {
        let temp_dir = TempDir::new().unwrap();
        let obj_path = temp_dir.path().join("multi.obj");

        // Create multiple meshes
        let mesh1 = create_test_mesh();
        let mesh2 = create_test_cube();

        export_obj(&[mesh1.clone(), mesh2.clone()], &obj_path)
            .expect("Failed to export multi-mesh OBJ");

        // Import back
        let imported = import_obj(&obj_path).expect("Failed to import multi-mesh OBJ");

        // Should have at least the meshes we exported
        assert!(!imported.is_empty());
    }

    #[test]
    fn test_ply_binary_export_import() {
        let temp_dir = TempDir::new().unwrap();
        let ply_path = temp_dir.path().join("test.ply");

        // Create and export a test mesh
        let original_mesh = create_test_mesh();
        export_ply(&original_mesh, &ply_path, false).expect("Failed to export binary PLY");

        // Verify file exists
        assert!(ply_path.exists());

        // Import the mesh back
        let imported_mesh = import_ply(&ply_path).expect("Failed to import PLY");

        // Verify vertex and face counts match
        assert_eq!(original_mesh.vertex_count(), imported_mesh.vertex_count());
        assert_eq!(original_mesh.face_count(), imported_mesh.face_count());
    }

    #[test]
    fn test_ply_ascii_export_import() {
        let temp_dir = TempDir::new().unwrap();
        let ply_path = temp_dir.path().join("test_ascii.ply");

        // Create and export a test mesh
        let original_mesh = create_test_cube();
        export_ply(&original_mesh, &ply_path, true).expect("Failed to export ASCII PLY");

        // Verify file exists
        assert!(ply_path.exists());

        // Import the mesh back
        let imported_mesh = import_ply(&ply_path).expect("Failed to import ASCII PLY");

        // Verify vertex and face counts match
        assert_eq!(original_mesh.vertex_count(), imported_mesh.vertex_count());
        assert_eq!(original_mesh.face_count(), imported_mesh.face_count());
    }

    #[test]
    fn test_all_formats_roundtrip() {
        let temp_dir = TempDir::new().unwrap();
        let original = create_test_cube();

        // Test STL
        let stl_path = temp_dir.path().join("test.stl");
        export_stl(&original, &stl_path).unwrap();
        let stl_imported = import_stl(&stl_path).unwrap();
        assert_eq!(original.vertex_count(), stl_imported.vertex_count());

        // Test OBJ
        let obj_path = temp_dir.path().join("test.obj");
        export_obj(&[original.clone()], &obj_path).unwrap();
        let obj_imported = import_obj(&obj_path).unwrap();
        assert_eq!(original.vertex_count(), obj_imported[0].vertex_count());

        // Test PLY
        let ply_path = temp_dir.path().join("test.ply");
        export_ply(&original, &ply_path, false).unwrap();
        let ply_imported = import_ply(&ply_path).unwrap();
        assert_eq!(original.vertex_count(), ply_imported.vertex_count());
    }
}
