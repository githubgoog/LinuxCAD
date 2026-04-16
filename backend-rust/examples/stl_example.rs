// Example demonstrating STL import/export functionality
//
// Run with: cargo run --example stl_example

use linuxcad_backend::import_export::{
    export_stl, export_stl_ascii, export_stl_binary, import_stl,
};
use linuxcad_backend::types::MeshData;
use nalgebra::{Point3, Vector3};
use std::path::Path;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    println!("STL Import/Export Example\n");

    // ─────────────────────────────────────────────────────────────────────
    // Example 1: Create a simple mesh and export to STL
    // ─────────────────────────────────────────────────────────────────────

    println!("1. Creating a simple triangle mesh...");
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

    println!("   Vertices: {}", mesh.vertex_count());
    println!("   Faces: {}", mesh.face_count());

    // ─────────────────────────────────────────────────────────────────────
    // Example 2: Export to binary STL (default, most efficient)
    // ─────────────────────────────────────────────────────────────────────

    println!("\n2. Exporting to binary STL...");
    export_stl(&mesh, "output/triangle_binary.stl")?;
    println!("   Saved to: output/triangle_binary.stl");

    // Or explicitly use binary export
    export_stl_binary(&mesh, "output/triangle_binary_explicit.stl")?;

    // ─────────────────────────────────────────────────────────────────────
    // Example 3: Export to ASCII STL (human-readable)
    // ─────────────────────────────────────────────────────────────────────

    println!("\n3. Exporting to ASCII STL...");
    export_stl_ascii(&mesh, "output/triangle_ascii.stl")?;
    println!("   Saved to: output/triangle_ascii.stl");

    // ─────────────────────────────────────────────────────────────────────
    // Example 4: Import an STL file (auto-detects format)
    // ─────────────────────────────────────────────────────────────────────

    println!("\n4. Importing STL file...");
    let imported_mesh = import_stl("output/triangle_binary.stl")?;
    println!("   Imported vertices: {}", imported_mesh.vertex_count());
    println!("   Imported faces: {}", imported_mesh.face_count());

    // ─────────────────────────────────────────────────────────────────────
    // Example 5: Create a more complex mesh (cube)
    // ─────────────────────────────────────────────────────────────────────

    println!("\n5. Creating a cube mesh...");
    let cube_vertices = vec![
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

    let cube_faces = vec![
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

    // Normals will be calculated automatically
    let cube_normals = vec![Vector3::new(0.0, 0.0, 1.0); 8]; // Placeholder
    let cube_mesh = MeshData::new(cube_vertices, cube_faces, cube_normals);

    println!("   Vertices: {}", cube_mesh.vertex_count());
    println!("   Faces: {}", cube_mesh.face_count());

    export_stl(&cube_mesh, "output/cube.stl")?;
    println!("   Saved to: output/cube.stl");

    // ─────────────────────────────────────────────────────────────────────
    // Example 6: Round-trip test (export and import)
    // ─────────────────────────────────────────────────────────────────────

    println!("\n6. Round-trip test...");
    export_stl(&cube_mesh, "output/roundtrip.stl")?;
    let roundtrip_mesh = import_stl("output/roundtrip.stl")?;

    println!(
        "   Original: {} vertices, {} faces",
        cube_mesh.vertex_count(),
        cube_mesh.face_count()
    );
    println!(
        "   After round-trip: {} vertices, {} faces",
        roundtrip_mesh.vertex_count(),
        roundtrip_mesh.face_count()
    );

    if cube_mesh.vertex_count() == roundtrip_mesh.vertex_count()
        && cube_mesh.face_count() == roundtrip_mesh.face_count()
    {
        println!("   ✓ Round-trip successful!");
    } else {
        println!("   ✗ Round-trip mismatch!");
    }

    println!("\nDone!");
    Ok(())
}
