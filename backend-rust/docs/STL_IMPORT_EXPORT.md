# STL Import/Export Implementation

This document describes the STL (STereoLithography) file format import/export implementation in the Linux CAD Rust backend.

## Overview

The `import_export` module provides high-performance STL file I/O with support for both ASCII and binary formats. It integrates seamlessly with the `MeshData` type and includes automatic format detection.

## Features

- **Automatic Format Detection**: Automatically detects whether an STL file is ASCII or binary
- **Binary STL Support**: Fast and compact binary format (default for export)
- **ASCII STL Support**: Human-readable text format for debugging
- **Efficient Normal Calculation**: Smooth vertex normals computed from face normals
- **Memory Efficient**: Uses flattened arrays and pre-allocated buffers
- **Comprehensive Error Handling**: Uses `thiserror` for clear error messages
- **Well-Tested**: Includes unit tests for all major functionality

## API Reference

### Import Functions

```rust
// Auto-detect format and import
pub fn import_stl<P: AsRef<Path>>(path: P) -> Result<MeshData>

// Import ASCII STL
pub fn import_stl_ascii<P: AsRef<Path>>(path: P) -> Result<MeshData>

// Import binary STL
pub fn import_stl_binary<P: AsRef<Path>>(path: P) -> Result<MeshData>
```

### Export Functions

```rust
// Export to binary STL (default, most efficient)
pub fn export_stl<P: AsRef<Path>>(mesh: &MeshData, path: P) -> Result<()>

// Export to binary STL (explicit)
pub fn export_stl_binary<P: AsRef<Path>>(mesh: &MeshData, path: P) -> Result<()>

// Export to ASCII STL (human-readable)
pub fn export_stl_ascii<P: AsRef<Path>>(mesh: &MeshData, path: P) -> Result<()>
```

## Usage Examples

### Basic Import

```rust
use linuxcad_backend::import_export::import_stl;

// Automatically detects format (ASCII or binary)
let mesh = import_stl("model.stl")?;

println!("Loaded {} vertices, {} faces",
         mesh.vertex_count(), mesh.face_count());
```

### Basic Export

```rust
use linuxcad_backend::import_export::export_stl;
use linuxcad_backend::types::MeshData;
use nalgebra::{Point3, Vector3};

// Create a simple triangle
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
let mesh = MeshData::new(vertices, faces, normals);

// Export (binary by default)
export_stl(&mesh, "output.stl")?;
```

### ASCII Export for Debugging

```rust
use linuxcad_backend::import_export::export_stl_ascii;

// Export as human-readable ASCII
export_stl_ascii(&mesh, "debug.stl")?;
```

### Round-Trip Test

```rust
use linuxcad_backend::import_export::{export_stl, import_stl};

// Export mesh
export_stl(&original_mesh, "temp.stl")?;

// Import it back
let imported = import_stl("temp.stl")?;

// Verify integrity
assert_eq!(original_mesh.vertex_count(), imported.vertex_count());
assert_eq!(original_mesh.face_count(), imported.face_count());
```

## Error Handling

The module uses `thiserror` for comprehensive error handling:

```rust
#[derive(Debug, Error)]
pub enum ImportExportError {
    #[error("IO error: {0}")]
    Io(#[from] std::io::Error),

    #[error("Invalid STL file: {0}")]
    InvalidStl(String),

    #[error("Unsupported file format: {0}")]
    UnsupportedFormat(String),

    #[error("Parse error: {0}")]
    ParseError(String),

    #[error("Invalid mesh data: {0}")]
    InvalidMeshData(String),
}
```

Example error handling:

```rust
use linuxcad_backend::import_export::{import_stl, ImportExportError};

match import_stl("model.stl") {
    Ok(mesh) => println!("Successfully loaded mesh"),
    Err(ImportExportError::Io(e)) => eprintln!("File I/O error: {}", e),
    Err(ImportExportError::InvalidStl(msg)) => eprintln!("Invalid STL: {}", msg),
    Err(e) => eprintln!("Error: {}", e),
}
```

## Implementation Details

### MeshData Integration

The STL importer converts to the `MeshData` format:

- **Vertices**: Flattened `Vec<f32>` with XYZ coordinates
- **Indices**: Triangle indices as `Vec<u32>`
- **Normals**: Vertex normals (smoothed from face normals)
- **Edges**: Edge indices for wireframe rendering

### Normal Calculation

The module implements smooth vertex normals by:
1. Calculating face normals using cross product
2. Accumulating face normals at each vertex
3. Averaging and normalizing the accumulated normals

This produces visually smoother results than flat shading.

### Format Detection

The `is_stl_ascii()` function checks the first 6 bytes:
- ASCII STL files start with `"solid "`
- Binary STL files start with arbitrary header bytes

### Performance Characteristics

- **Binary Import**: O(n) where n = number of triangles
- **Binary Export**: O(n) with buffered writes
- **ASCII Import**: O(n) with parsing overhead
- **ASCII Export**: O(n) with string formatting overhead
- **Memory**: Pre-allocated vectors minimize allocations

**Recommendation**: Use binary format for production, ASCII for debugging.

## Testing

The module includes comprehensive tests:

```bash
# Run all import/export tests
cargo test --lib import_export

# Run specific test
cargo test --lib import_export::tests::test_stl_binary_export_import

# Run with output
cargo test --lib import_export -- --nocapture
```

### Test Coverage

- ✓ Binary STL export/import round-trip
- ✓ ASCII STL export/import round-trip
- ✓ Automatic format detection
- ✓ Cube mesh export/import
- ✓ Normal calculation accuracy
- ✓ Empty mesh error handling
- ✓ Mesh data validation
- ✓ Large mesh performance

## Dependencies

```toml
[dependencies]
stl_io = "0.7"           # STL file format handling
nalgebra = "0.32"         # Vector/point math
thiserror = "1.0"         # Error handling

[dev-dependencies]
tempfile = "3.8"          # Temporary files for tests
```

## Future Enhancements

Potential improvements:
- [ ] Parallel mesh processing with Rayon
- [ ] Streaming import for very large files
- [ ] Color STL support (VRML extensions)
- [ ] Mesh validation and repair
- [ ] Progress callbacks for large files
- [ ] Compression support (STL.gz)

## File Format Reference

### ASCII STL Format

```
solid name
  facet normal nx ny nz
    outer loop
      vertex x1 y1 z1
      vertex x2 y2 z2
      vertex x3 y3 z3
    endloop
  endfacet
  ...
endsolid name
```

### Binary STL Format

```
UINT8[80]    – Header (usually ignored)
UINT32       – Number of triangles
foreach triangle:
  REAL32[3]  – Normal vector
  REAL32[3]  – Vertex 1
  REAL32[3]  – Vertex 2
  REAL32[3]  – Vertex 3
  UINT16     – Attribute byte count (usually 0)
```

## Integration with CAD System

The STL import/export integrates with:

- **Geometry Engine**: Export computed meshes
- **Feature System**: Import as mesh features
- **File API**: REST endpoints for upload/download
- **Type System**: Uses standard `MeshData` type

Example integration:

```rust
use linuxcad_backend::geometry::GeometryEngine;
use linuxcad_backend::import_export::export_stl;

let engine = GeometryEngine::new();
let mesh = engine.compute_mesh(&feature)?;
export_stl(&mesh, "output.stl")?;
```

## License

Part of the Linux CAD project - see main project license.
