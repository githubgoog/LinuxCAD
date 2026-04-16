use crate::types::*;
use nalgebra::{Point3, Vector3};
use parry3d_f64::query::Ray;
use parry3d_f64::shape::TriMesh;
use std::collections::HashMap;
use tracing::{debug, instrument, warn};

/// High-performance boolean operations engine for LinuxCAD
pub struct BooleanEngine {
    /// Intersection tolerance for floating point operations
    tolerance: f64,
}

impl BooleanEngine {
    pub fn new() -> Self {
        Self {
            tolerance: 1e-6,
        }
    }

    /// Perform union operation on two meshes
    #[instrument(skip(self, mesh_a, mesh_b))]
    pub fn union(&self, mesh_a: &MeshData, mesh_b: &MeshData) -> Result<MeshData, GeometryError> {
        debug!("Computing union of meshes with {} and {} vertices",
               mesh_a.vertex_count(), mesh_b.vertex_count());

        // Convert MeshData to parry3d TriMesh for processing
        let trimesh_a = self.mesh_data_to_trimesh(mesh_a)?;
        let trimesh_b = self.mesh_data_to_trimesh(mesh_b)?;

        // Perform CSG union
        let result = self.csg_union(&trimesh_a, &trimesh_b)?;

        debug!("Union result: {} vertices", result.vertex_count());
        Ok(result)
    }

    /// Perform subtraction operation (A - B)
    #[instrument(skip(self, mesh_a, mesh_b))]
    pub fn subtract(&self, mesh_a: &MeshData, mesh_b: &MeshData) -> Result<MeshData, GeometryError> {
        debug!("Computing subtraction of meshes with {} and {} vertices",
               mesh_a.vertex_count(), mesh_b.vertex_count());

        let trimesh_a = self.mesh_data_to_trimesh(mesh_a)?;
        let trimesh_b = self.mesh_data_to_trimesh(mesh_b)?;

        let result = self.csg_subtract(&trimesh_a, &trimesh_b)?;

        debug!("Subtraction result: {} vertices", result.vertex_count());
        Ok(result)
    }

    /// Perform intersection operation
    #[instrument(skip(self, mesh_a, mesh_b))]
    pub fn intersect(&self, mesh_a: &MeshData, mesh_b: &MeshData) -> Result<MeshData, GeometryError> {
        debug!("Computing intersection of meshes with {} and {} vertices",
               mesh_a.vertex_count(), mesh_b.vertex_count());

        let trimesh_a = self.mesh_data_to_trimesh(mesh_a)?;
        let trimesh_b = self.mesh_data_to_trimesh(mesh_b)?;

        let result = self.csg_intersect(&trimesh_a, &trimesh_b)?;

        debug!("Intersection result: {} vertices", result.vertex_count());
        Ok(result)
    }

    /// Convert MeshData to parry3d TriMesh
    fn mesh_data_to_trimesh(&self, mesh: &MeshData) -> Result<TriMesh, GeometryError> {
        if mesh.vertices.len() % 3 != 0 {
            return Err(GeometryError::InvalidParameters(
                "Vertices array must be a multiple of 3".to_string(),
            ));
        }

        if mesh.indices.len() % 3 != 0 {
            return Err(GeometryError::InvalidParameters(
                "Indices array must be a multiple of 3".to_string(),
            ));
        }

        // Convert vertices to Point3
        let vertices: Vec<Point3<f64>> = mesh.vertices
            .chunks_exact(3)
            .map(|chunk| Point3::new(chunk[0] as f64, chunk[1] as f64, chunk[2] as f64))
            .collect();

        // Convert indices to triangles
        let indices: Vec<[u32; 3]> = mesh.indices
            .chunks_exact(3)
            .map(|chunk| [chunk[0], chunk[1], chunk[2]])
            .collect();

        Ok(TriMesh::new(vertices, indices))
    }

    /// Convert TriMesh back to MeshData
    fn trimesh_to_mesh_data(&self, trimesh: &TriMesh) -> MeshData {
        // Extract vertices
        let vertices: Vec<f32> = trimesh.vertices()
            .iter()
            .flat_map(|v| [v.x as f32, v.y as f32, v.z as f32])
            .collect();

        // Extract indices
        let indices: Vec<u32> = trimesh.indices()
            .iter()
            .flat_map(|triangle| [triangle[0], triangle[1], triangle[2]])
            .collect();

        // Calculate vertex normals
        let vertex_points: Vec<Point3<f32>> = trimesh
            .vertices()
            .iter()
            .map(|v| Point3::new(v.x as f32, v.y as f32, v.z as f32))
            .collect();
        let face_indices: Vec<[usize; 3]> = trimesh.indices()
            .iter()
            .map(|tri| [tri[0] as usize, tri[1] as usize, tri[2] as usize])
            .collect();

        let normals = crate::geometry::calculate_vertex_normals(&vertex_points, &face_indices);
        let normals_flat: Vec<f32> = normals
            .iter()
            .flat_map(|n| [n.x, n.y, n.z])
            .collect();

        // Generate edge indices for wireframe
        let mut edges = Vec::new();
        for triangle in trimesh.indices() {
            edges.extend_from_slice(&[
                triangle[0], triangle[1],
                triangle[1], triangle[2],
                triangle[2], triangle[0],
            ]);
        }

        MeshData {
            vertices,
            indices,
            normals: normals_flat,
            edges,
        }
    }

    /// Efficient CSG Union using ray casting and mesh merging
    fn csg_union(&self, mesh_a: &TriMesh, mesh_b: &TriMesh) -> Result<MeshData, GeometryError> {
        // For union: keep all exterior faces from both meshes
        let faces_a = self.extract_exterior_faces(mesh_a, mesh_b, false)?;
        let faces_b = self.extract_exterior_faces(mesh_b, mesh_a, false)?;

        self.merge_face_sets(faces_a, faces_b)
    }

    /// Efficient CSG Subtraction using ray casting
    fn csg_subtract(&self, mesh_a: &TriMesh, mesh_b: &TriMesh) -> Result<MeshData, GeometryError> {
        // For subtraction: keep exterior faces of A that are outside B,
        // and interior faces of B that are inside A (inverted)
        let faces_a = self.extract_exterior_faces(mesh_a, mesh_b, false)?;
        let faces_b = self.extract_exterior_faces(mesh_b, mesh_a, true)?; // Inverted

        self.merge_face_sets(faces_a, faces_b)
    }

    /// Efficient CSG Intersection using ray casting
    fn csg_intersect(&self, mesh_a: &TriMesh, mesh_b: &TriMesh) -> Result<MeshData, GeometryError> {
        // For intersection: keep interior faces from both meshes
        let faces_a = self.extract_interior_faces(mesh_a, mesh_b)?;
        let faces_b = self.extract_interior_faces(mesh_b, mesh_a)?;

        self.merge_face_sets(faces_a, faces_b)
    }

    /// Extract exterior faces (outside the other mesh)
    fn extract_exterior_faces(
        &self,
        source_mesh: &TriMesh,
        test_mesh: &TriMesh,
        invert_normals: bool,
    ) -> Result<Vec<Triangle>, GeometryError> {
        let mut exterior_faces = Vec::new();
        let test_aabb = test_mesh.local_aabb();

        for (i, triangle) in source_mesh.indices().iter().enumerate() {
            let v0 = source_mesh.vertices()[triangle[0] as usize];
            let v1 = source_mesh.vertices()[triangle[1] as usize];
            let v2 = source_mesh.vertices()[triangle[2] as usize];

            // Calculate face centroid for testing
            let centroid = Point3::new(
                (v0.x + v1.x + v2.x) / 3.0,
                (v0.y + v1.y + v2.y) / 3.0,
                (v0.z + v1.z + v2.z) / 3.0,
            );

            // Quick AABB test first
            if !test_aabb.contains_local_point(&centroid) {
                // Definitely exterior
                exterior_faces.push(Triangle::new(v0, v1, v2, invert_normals));
                continue;
            }

            // Ray casting test for points inside AABB
            if !self.is_point_inside_mesh(centroid, test_mesh)? {
                exterior_faces.push(Triangle::new(v0, v1, v2, invert_normals));
            }
        }

        Ok(exterior_faces)
    }

    /// Extract interior faces (inside the other mesh)
    fn extract_interior_faces(
        &self,
        source_mesh: &TriMesh,
        test_mesh: &TriMesh,
    ) -> Result<Vec<Triangle>, GeometryError> {
        let mut interior_faces = Vec::new();
        let test_aabb = test_mesh.local_aabb();

        for (i, triangle) in source_mesh.indices().iter().enumerate() {
            let v0 = source_mesh.vertices()[triangle[0] as usize];
            let v1 = source_mesh.vertices()[triangle[1] as usize];
            let v2 = source_mesh.vertices()[triangle[2] as usize];

            let centroid = Point3::new(
                (v0.x + v1.x + v2.x) / 3.0,
                (v0.y + v1.y + v2.y) / 3.0,
                (v0.z + v1.z + v2.z) / 3.0,
            );

            // Only test points that could be inside
            if test_aabb.contains_local_point(&centroid) {
                if self.is_point_inside_mesh(centroid, test_mesh)? {
                    interior_faces.push(Triangle::new(v0, v1, v2, false));
                }
            }
        }

        Ok(interior_faces)
    }

    /// Test if a point is inside a mesh using ray casting
    fn is_point_inside_mesh(&self, point: Point3<f64>, mesh: &TriMesh) -> Result<bool, GeometryError> {
        // Cast ray in positive X direction
        let ray = Ray::new(point, Vector3::new(1.0, 0.0, 0.0));

        let mut intersection_count = 0;

        // Count intersections with mesh faces
        for triangle in mesh.indices() {
            let v0 = mesh.vertices()[triangle[0] as usize];
            let v1 = mesh.vertices()[triangle[1] as usize];
            let v2 = mesh.vertices()[triangle[2] as usize];

            if self.ray_triangle_intersect(&ray, v0, v1, v2)? {
                intersection_count += 1;
            }
        }

        // Odd number of intersections = inside
        Ok(intersection_count % 2 == 1)
    }

    /// Ray-triangle intersection test
    fn ray_triangle_intersect(
        &self,
        ray: &Ray,
        v0: Point3<f64>,
        v1: Point3<f64>,
        v2: Point3<f64>,
    ) -> Result<bool, GeometryError> {
        // Möller-Trumbore intersection algorithm
        let edge1 = v1 - v0;
        let edge2 = v2 - v0;
        let h = ray.dir.cross(&edge2);
        let a = edge1.dot(&h);

        if a.abs() < self.tolerance {
            return Ok(false); // Ray parallel to triangle
        }

        let f = 1.0 / a;
        let s = ray.origin - v0;
        let u = f * s.dot(&h);

        if u < 0.0 || u > 1.0 {
            return Ok(false);
        }

        let q = s.cross(&edge1);
        let v = f * ray.dir.dot(&q);

        if v < 0.0 || u + v > 1.0 {
            return Ok(false);
        }

        let t = f * edge2.dot(&q);
        Ok(t > self.tolerance) // Intersection ahead of ray origin
    }

    /// Merge two sets of triangles into a single mesh
    fn merge_face_sets(&self, faces_a: Vec<Triangle>, faces_b: Vec<Triangle>) -> Result<MeshData, GeometryError> {
        let mut vertices = Vec::new();
        let mut indices = Vec::new();
        let mut vertex_map: HashMap<[i64; 3], u32> = HashMap::new();

        let mut add_triangle = |triangle: &Triangle| {
            let vertices_to_add = [triangle.v0, triangle.v1, triangle.v2];
            let mut triangle_indices = [0u32; 3];

            for (i, vertex) in vertices_to_add.iter().enumerate() {
                // Create a unique key for the vertex (quantized to handle floating point precision)
                let key = [
                    (vertex.x / self.tolerance).round() as i64,
                    (vertex.y / self.tolerance).round() as i64,
                    (vertex.z / self.tolerance).round() as i64,
                ];

                if let Some(&existing_index) = vertex_map.get(&key) {
                    triangle_indices[i] = existing_index;
                } else {
                    let new_index = vertices.len() as u32;
                    vertices.extend_from_slice(&[vertex.x as f32, vertex.y as f32, vertex.z as f32]);
                    vertex_map.insert(key, new_index);
                    triangle_indices[i] = new_index;
                }
            }

            indices.extend_from_slice(&triangle_indices);
        };

        // Add all triangles from both sets
        for face in faces_a {
            add_triangle(&face);
        }
        for face in faces_b {
            add_triangle(&face);
        }

        if vertices.is_empty() {
            return Err(GeometryError::BooleanOperationFailed(
                "Boolean operation resulted in empty mesh".to_string(),
            ));
        }

        // Calculate normals and edges
        let vertex_points: Vec<Point3<f32>> = vertices
            .chunks_exact(3)
            .map(|chunk| Point3::new(chunk[0], chunk[1], chunk[2]))
            .collect();

        let face_indices: Vec<[usize; 3]> = indices
            .chunks_exact(3)
            .map(|chunk| [chunk[0] as usize, chunk[1] as usize, chunk[2] as usize])
            .collect();

        let normals = crate::geometry::calculate_vertex_normals(&vertex_points, &face_indices);
        let normals_flat: Vec<f32> = normals
            .iter()
            .flat_map(|n| [n.x, n.y, n.z])
            .collect();

        // Generate edges
        let mut edges = Vec::new();
        for triangle in indices.chunks_exact(3) {
            edges.extend_from_slice(&[
                triangle[0], triangle[1],
                triangle[1], triangle[2],
                triangle[2], triangle[0],
            ]);
        }

        Ok(MeshData {
            vertices,
            indices,
            normals: normals_flat,
            edges,
        })
    }
}

/// Helper struct for triangle data during boolean operations
#[derive(Debug, Clone)]
struct Triangle {
    v0: Point3<f64>,
    v1: Point3<f64>,
    v2: Point3<f64>,
}

impl Triangle {
    fn new(v0: Point3<f64>, v1: Point3<f64>, v2: Point3<f64>, invert_normals: bool) -> Self {
        if invert_normals {
            Self { v0, v1: v2, v2: v1 } // Reverse winding order
        } else {
            Self { v0, v1, v2 }
        }
    }
}

impl Default for BooleanEngine {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::geometry::*;

    fn create_cube_mesh(center: Point3<f32>, size: f32) -> MeshData {
        let half = size / 2.0;
        let vertices = vec![
            // Front face
            center.x - half, center.y - half, center.z + half,
            center.x + half, center.y - half, center.z + half,
            center.x + half, center.y + half, center.z + half,
            center.x - half, center.y + half, center.z + half,
            // Back face
            center.x - half, center.y - half, center.z - half,
            center.x + half, center.y - half, center.z - half,
            center.x + half, center.y + half, center.z - half,
            center.x - half, center.y + half, center.z - half,
        ];

        let indices = vec![
            0, 1, 2, 2, 3, 0, // Front
            1, 5, 6, 6, 2, 1, // Right
            5, 4, 7, 7, 6, 5, // Back
            4, 0, 3, 3, 7, 4, // Left
            3, 2, 6, 6, 7, 3, // Top
            4, 5, 1, 1, 0, 4, // Bottom
        ];

        let vertex_points: Vec<Point3<f32>> = vertices
            .chunks_exact(3)
            .map(|chunk| Point3::new(chunk[0], chunk[1], chunk[2]))
            .collect();

        let face_indices: Vec<[usize; 3]> = indices
            .chunks_exact(3)
            .map(|chunk| [chunk[0], chunk[1], chunk[2]])
            .collect();

        let normals = calculate_vertex_normals(&vertex_points, &face_indices);
        let normals_flat: Vec<f32> = normals
            .iter()
            .flat_map(|n| [n.x, n.y, n.z])
            .collect();

        let edges: Vec<u32> = indices
            .chunks_exact(3)
            .flat_map(|triangle| [
                triangle[0], triangle[1],
                triangle[1], triangle[2],
                triangle[2], triangle[0],
            ])
            .collect();

        MeshData {
            vertices,
            indices,
            normals: normals_flat,
            edges,
        }
    }

    #[test]
    fn test_boolean_union() {
        let engine = BooleanEngine::new();

        let cube_a = create_cube_mesh(Point3::new(0.0, 0.0, 0.0), 2.0);
        let cube_b = create_cube_mesh(Point3::new(1.0, 0.0, 0.0), 2.0);

        let result = engine.union(&cube_a, &cube_b);
        assert!(result.is_ok());

        let union_mesh = result.unwrap();
        assert!(union_mesh.vertex_count() > 0);
        assert!(union_mesh.face_count() > 0);
    }

    #[test]
    fn test_boolean_subtract() {
        let engine = BooleanEngine::new();

        let cube_a = create_cube_mesh(Point3::new(0.0, 0.0, 0.0), 2.0);
        let cube_b = create_cube_mesh(Point3::new(0.5, 0.0, 0.0), 1.0);

        let result = engine.subtract(&cube_a, &cube_b);
        assert!(result.is_ok());

        let subtract_mesh = result.unwrap();
        assert!(subtract_mesh.vertex_count() > 0);
    }

    #[test]
    fn test_boolean_intersect() {
        let engine = BooleanEngine::new();

        let cube_a = create_cube_mesh(Point3::new(0.0, 0.0, 0.0), 2.0);
        let cube_b = create_cube_mesh(Point3::new(0.5, 0.0, 0.0), 2.0);

        let result = engine.intersect(&cube_a, &cube_b);
        assert!(result.is_ok());

        let intersect_mesh = result.unwrap();
        assert!(intersect_mesh.vertex_count() > 0);
    }
}