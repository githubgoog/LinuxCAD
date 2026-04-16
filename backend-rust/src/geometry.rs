use crate::types::*;
use nalgebra::{Point3, Vector3};
use std::collections::HashMap;
use std::f64::consts::PI;
use tracing::{debug, instrument, warn};

/// High-performance geometry engine for LinuxCAD
pub struct GeometryEngine {
    // Remove the function pointer hashmap approach
}

impl GeometryEngine {
    pub fn new() -> Self {
        Self {}
    }

    /// Generate mesh for a feature with high performance
    #[instrument(skip(self, feature))]
    pub fn compute_mesh(&self, feature: &Feature) -> Result<MeshData, GeometryError> {
        match &feature.feature_type {
            FeatureType::Primitive(primitive_type) => {
                let mut mesh = match primitive_type {
                    PrimitiveType::Box => build_box(&feature.params)?,
                    PrimitiveType::Cylinder => build_cylinder(&feature.params)?,
                    PrimitiveType::Sphere => build_sphere(&feature.params)?,
                    PrimitiveType::Cone => build_cone(&feature.params)?,
                    PrimitiveType::Torus => build_torus(&feature.params)?,
                    PrimitiveType::Wedge => build_wedge(&feature.params)?,
                    PrimitiveType::Tube => build_tube(&feature.params)?,
                    PrimitiveType::Pyramid => build_pyramid(&feature.params)?,
                    PrimitiveType::Ellipsoid => build_ellipsoid(&feature.params)?,
                    _ => {
                        return Err(GeometryError::InvalidParameters(format!(
                            "Unsupported primitive type: {:?}",
                            primitive_type
                        )))
                    }
                };

                // Apply transformations
                self.apply_transform(&mut mesh, &feature.params)?;

                debug!(
                    "Generated mesh for {} with {} vertices, {} faces",
                    feature.name,
                    mesh.vertex_count(),
                    mesh.face_count()
                );

                Ok(mesh)
            }
            FeatureType::Advanced(advanced_type) => {
                // Handle advanced operations
                match advanced_type {
                    AdvancedType::Shell => self.compute_shell_operation(feature),
                    _ => Err(GeometryError::InvalidParameters(format!(
                        "Advanced operation not yet implemented: {:?}",
                        advanced_type
                    ))),
                }
            }
        }
    }

    /// Apply position, rotation, and scale transforms
    #[instrument(skip(self, mesh, params))]
    fn apply_transform(
        &self,
        mesh: &mut MeshData,
        params: &HashMap<String, f64>,
    ) -> Result<(), GeometryError> {
        let pos_x = *params.get("posX").unwrap_or(&0.0);
        let pos_y = *params.get("posY").unwrap_or(&0.0);
        let pos_z = *params.get("posZ").unwrap_or(&0.0);

        let rot_x = *params.get("rotX").unwrap_or(&0.0) * PI / 180.0;
        let rot_y = *params.get("rotY").unwrap_or(&0.0) * PI / 180.0;
        let rot_z = *params.get("rotZ").unwrap_or(&0.0) * PI / 180.0;

        // Create transformation matrix
        let translation = nalgebra::Translation3::new(pos_x as f32, pos_y as f32, pos_z as f32);
        let rotation =
            nalgebra::Rotation3::from_euler_angles(rot_x as f32, rot_y as f32, rot_z as f32);
        let transform = nalgebra::Isometry3::from_parts(translation, rotation.into());

        // Apply transformation to vertices
        for i in 0..(mesh.vertices.len() / 3) {
            let vertex = Point3::new(
                mesh.vertices[i * 3],
                mesh.vertices[i * 3 + 1],
                mesh.vertices[i * 3 + 2],
            );

            let transformed = transform * vertex;

            mesh.vertices[i * 3] = transformed.x;
            mesh.vertices[i * 3 + 1] = transformed.y;
            mesh.vertices[i * 3 + 2] = transformed.z;
        }

        // Apply transformation to normals (rotation only)
        for i in 0..(mesh.normals.len() / 3) {
            let normal = Vector3::new(
                mesh.normals[i * 3],
                mesh.normals[i * 3 + 1],
                mesh.normals[i * 3 + 2],
            );

            let transformed = rotation * normal;

            mesh.normals[i * 3] = transformed.x;
            mesh.normals[i * 3 + 1] = transformed.y;
            mesh.normals[i * 3 + 2] = transformed.z;
        }

        Ok(())
    }

    fn compute_shell_operation(&self, _feature: &Feature) -> Result<MeshData, GeometryError> {
        // Placeholder for shell operation
        Err(GeometryError::InvalidParameters(
            "Shell operation not yet implemented".to_string(),
        ))
    }

    pub fn shell_feature(
        &self,
        feature: &Feature,
        thickness: f64,
    ) -> Result<MeshData, GeometryError> {
        let outer = self.compute_mesh(feature)?;
        let mut vertices = outer.vertices.clone();
        let mut indices = outer.indices.clone();

        let mut cx = 0.0f32;
        let mut cy = 0.0f32;
        let mut cz = 0.0f32;
        let count = outer.vertex_count().max(1) as f32;
        for i in 0..outer.vertex_count() {
            cx += outer.vertices[i * 3];
            cy += outer.vertices[i * 3 + 1];
            cz += outer.vertices[i * 3 + 2];
        }
        cx /= count;
        cy /= count;
        cz /= count;

        let t = (thickness as f32).max(0.0001);
        let inner_offset = outer.vertex_count() as u32;
        for i in 0..outer.vertex_count() {
            let px = outer.vertices[i * 3];
            let py = outer.vertices[i * 3 + 1];
            let pz = outer.vertices[i * 3 + 2];
            let mut dx = px - cx;
            let mut dy = py - cy;
            let mut dz = pz - cz;
            let len = (dx * dx + dy * dy + dz * dz).sqrt().max(1e-6);
            dx /= len;
            dy /= len;
            dz /= len;

            vertices.push(px - dx * t);
            vertices.push(py - dy * t);
            vertices.push(pz - dz * t);
        }

        for tri in outer.indices.chunks(3) {
            indices.push(inner_offset + tri[0]);
            indices.push(inner_offset + tri[2]);
            indices.push(inner_offset + tri[1]);
        }

        let mut shell = MeshData {
            vertices,
            indices,
            normals: Vec::new(),
            edges: Vec::new(),
        };
        rebuild_edges(&mut shell);
        recompute_mesh_normals(&mut shell)?;
        shell.validate()?;
        Ok(shell)
    }

    pub fn cut_feature(
        &self,
        feature: &Feature,
        axis: &str,
        offset: f64,
        keep_side: &str,
    ) -> Result<MeshData, GeometryError> {
        let source = self.compute_mesh(feature)?;
        let axis_idx = axis_index(axis)?;
        let keep_positive = !keep_side.eq_ignore_ascii_case("negative");

        let mut indices = Vec::with_capacity(source.indices.len());
        for tri in source.indices.chunks(3) {
            let a = tri[0] as usize;
            let b = tri[1] as usize;
            let c = tri[2] as usize;

            let ax = source.vertices[a * 3];
            let ay = source.vertices[a * 3 + 1];
            let az = source.vertices[a * 3 + 2];
            let bx = source.vertices[b * 3];
            let by = source.vertices[b * 3 + 1];
            let bz = source.vertices[b * 3 + 2];
            let cx = source.vertices[c * 3];
            let cy = source.vertices[c * 3 + 1];
            let cz = source.vertices[c * 3 + 2];

            let center_component = match axis_idx {
                0 => (ax + bx + cx) / 3.0,
                1 => (ay + by + cy) / 3.0,
                _ => (az + bz + cz) / 3.0,
            };

            let keep = if keep_positive {
                center_component >= offset as f32
            } else {
                center_component <= offset as f32
            };

            if keep {
                indices.extend_from_slice(tri);
            }
        }

        if indices.is_empty() {
            return Err(GeometryError::MeshGenerationFailed(
                "Cut removed all geometry; adjust offset or side".to_string(),
            ));
        }

        let mut out = MeshData {
            vertices: source.vertices,
            indices,
            normals: Vec::new(),
            edges: Vec::new(),
        };
        rebuild_edges(&mut out);
        recompute_mesh_normals(&mut out)?;
        out.validate()?;
        Ok(out)
    }

    pub fn revolve_feature(
        &self,
        feature: &Feature,
        axis: &str,
        side: &str,
        distance: f64,
        angle: f64,
        _segments: usize,
    ) -> Result<MeshData, GeometryError> {
        let mut mesh = self.compute_mesh(feature)?;
        apply_revolve_deformation(&mut mesh, axis, side, distance as f32, angle as f32)?;
        mesh.validate()?;
        Ok(mesh)
    }

    pub fn sweep_feature(
        &self,
        feature: &Feature,
        axis: &str,
        side: &str,
        distance: f64,
        start_angle: f64,
        end_angle: f64,
        path_rotate: f64,
        twist: f64,
        _segments: usize,
    ) -> Result<MeshData, GeometryError> {
        let mut mesh = self.compute_mesh(feature)?;
        apply_sweep_deformation(
            &mut mesh,
            axis,
            side,
            distance as f32,
            start_angle as f32,
            end_angle as f32,
            path_rotate as f32,
            twist as f32,
        )?;
        mesh.validate()?;
        Ok(mesh)
    }

    pub fn fillet_feature(
        &self,
        feature: &Feature,
        radius: f64,
        iterations: usize,
    ) -> Result<MeshData, GeometryError> {
        let mut mesh = self.compute_mesh(feature)?;
        let neighbors = build_vertex_neighbors(&mesh.indices, mesh.vertex_count());
        let strength = (radius as f32 / 10.0).clamp(0.01, 0.35);
        let passes = iterations.clamp(1, 8);
        laplacian_smooth(&mut mesh, &neighbors, strength, passes);
        recompute_mesh_normals(&mut mesh)?;
        mesh.validate()?;
        Ok(mesh)
    }

    pub fn chamfer_feature(
        &self,
        feature: &Feature,
        amount: f64,
    ) -> Result<MeshData, GeometryError> {
        let mut mesh = self.compute_mesh(feature)?;
        let centroid = mesh_centroid(&mesh);
        let amt = (amount as f32).clamp(0.001, 5.0);

        for i in 0..mesh.vertex_count() {
            let px = mesh.vertices[i * 3];
            let py = mesh.vertices[i * 3 + 1];
            let pz = mesh.vertices[i * 3 + 2];
            let mut dx = px - centroid.x;
            let mut dy = py - centroid.y;
            let mut dz = pz - centroid.z;
            let len = (dx * dx + dy * dy + dz * dz).sqrt().max(1e-6);
            dx /= len;
            dy /= len;
            dz /= len;
            mesh.vertices[i * 3] = px - dx * amt * 0.25;
            mesh.vertices[i * 3 + 1] = py - dy * amt * 0.25;
            mesh.vertices[i * 3 + 2] = pz - dz * amt * 0.25;
        }

        recompute_mesh_normals(&mut mesh)?;
        mesh.validate()?;
        Ok(mesh)
    }

    pub fn loft_features(
        &self,
        feature_a: &Feature,
        feature_b: &Feature,
        sections: usize,
        affect_ratio: f64,
    ) -> Result<MeshData, GeometryError> {
        let mesh_a = self.compute_mesh(feature_a)?;
        let mesh_b = self.compute_mesh(feature_b)?;

        let a_points = sample_mesh_vertices(&mesh_a, 32);
        let b_points = sample_mesh_vertices(&mesh_b, 32);
        if a_points.len() < 3 || b_points.len() < 3 {
            return Err(GeometryError::MeshGenerationFailed(
                "Loft requires enough source vertices".to_string(),
            ));
        }

        let ring = a_points.len().min(b_points.len());
        let rings = sections.clamp(2, 32) + 1;
        let ratio = (affect_ratio as f32).clamp(0.05, 0.95);

        let mut vertices = mesh_a.vertices.clone();
        let mut indices = mesh_a.indices.clone();
        let base_b = (vertices.len() / 3) as u32;
        vertices.extend_from_slice(&mesh_b.vertices);
        indices.extend(mesh_b.indices.iter().map(|idx| idx + base_b));

        let bridge_base = (vertices.len() / 3) as u32;
        for r in 0..rings {
            let t = (r as f32 / (rings - 1) as f32) * ratio;
            for i in 0..ring {
                let pa = a_points[i];
                let pb = b_points[i];
                let x = pa.x + (pb.x - pa.x) * t;
                let y = pa.y + (pb.y - pa.y) * t;
                let z = pa.z + (pb.z - pa.z) * t;
                vertices.extend_from_slice(&[x, y, z]);
            }
        }

        for r in 0..(rings - 1) {
            let row0 = bridge_base + (r * ring) as u32;
            let row1 = bridge_base + ((r + 1) * ring) as u32;
            for i in 0..ring {
                let n = (i + 1) % ring;
                let a = row0 + i as u32;
                let b = row0 + n as u32;
                let c = row1 + n as u32;
                let d = row1 + i as u32;
                indices.extend_from_slice(&[a, b, c, a, c, d]);
            }
        }

        let mut out = MeshData {
            vertices,
            indices,
            normals: Vec::new(),
            edges: Vec::new(),
        };
        rebuild_edges(&mut out);
        recompute_mesh_normals(&mut out)?;
        out.validate()?;
        Ok(out)
    }
}

impl Default for GeometryEngine {
    fn default() -> Self {
        Self::new()
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Primitive Builders (High-Performance)
// ─────────────────────────────────────────────────────────────────────────────

fn build_box(params: &HashMap<String, f64>) -> Result<MeshData, GeometryError> {
    let width = *params.get("width").unwrap_or(&2.0);
    let height = *params.get("height").unwrap_or(&2.0);
    let depth = *params.get("depth").unwrap_or(&2.0);

    let hw = width as f32 / 2.0;
    let hh = height as f32 / 2.0;
    let hd = depth as f32 / 2.0;

    // Manually create box vertices
    let vertices = vec![
        // Front face
        Point3::new(-hw, -hh, hd), // 0
        Point3::new(hw, -hh, hd),  // 1
        Point3::new(hw, hh, hd),   // 2
        Point3::new(-hw, hh, hd),  // 3
        // Back face
        Point3::new(-hw, -hh, -hd), // 4
        Point3::new(hw, -hh, -hd),  // 5
        Point3::new(hw, hh, -hd),   // 6
        Point3::new(-hw, hh, -hd),  // 7
    ];

    let indices = vec![
        // Front face
        [0, 1, 2],
        [0, 2, 3],
        // Back face
        [4, 6, 5],
        [4, 7, 6],
        // Right face
        [1, 5, 6],
        [1, 6, 2],
        // Left face
        [4, 0, 3],
        [4, 3, 7],
        // Top face
        [3, 2, 6],
        [3, 6, 7],
        // Bottom face
        [4, 5, 1],
        [4, 1, 0],
    ];

    let normals = calculate_vertex_normals(&vertices, &indices);

    Ok(MeshData::new(vertices, indices, normals))
}

fn build_cylinder(params: &HashMap<String, f64>) -> Result<MeshData, GeometryError> {
    let radius = *params.get("radius").unwrap_or(&1.0);
    let height = *params.get("height").unwrap_or(&2.0);
    let segments = *params.get("segments").unwrap_or(&32.0) as usize;

    let mut vertices = Vec::new();
    let mut indices = Vec::new();

    let r = radius as f32;
    let h = height as f32;
    let half_height = h / 2.0;

    // Generate vertices
    for i in 0..segments {
        let angle = (i as f32 / segments as f32) * 2.0 * PI as f32;
        let x = r * angle.cos();
        let z = r * angle.sin();

        // Bottom circle
        vertices.push(Point3::new(x, -half_height, z));
        // Top circle
        vertices.push(Point3::new(x, half_height, z));
    }

    // Add center points for caps
    vertices.push(Point3::new(0.0, -half_height, 0.0)); // Bottom center
    vertices.push(Point3::new(0.0, half_height, 0.0)); // Top center

    let bottom_center = segments * 2;
    let top_center = segments * 2 + 1;

    // Generate side faces
    for i in 0..segments {
        let next = (i + 1) % segments;
        let bottom1 = i * 2;
        let top1 = i * 2 + 1;
        let bottom2 = next * 2;
        let top2 = next * 2 + 1;

        // Side quad as two triangles
        indices.push([bottom1, bottom2, top2]);
        indices.push([bottom1, top2, top1]);

        // Bottom cap
        indices.push([bottom_center, bottom2, bottom1]);

        // Top cap
        indices.push([top_center, top1, top2]);
    }

    let normals = calculate_vertex_normals(&vertices, &indices);

    Ok(MeshData::new(vertices, indices, normals))
}

fn build_sphere(params: &HashMap<String, f64>) -> Result<MeshData, GeometryError> {
    let radius = *params.get("radius").unwrap_or(&1.0);
    let segments = *params.get("segments").unwrap_or(&32.0) as usize;
    let rings = *params.get("rings").unwrap_or(&16.0) as usize;

    let mut vertices = Vec::new();
    let mut indices = Vec::new();

    let r = radius as f32;

    // Generate vertices
    for i in 0..=rings {
        let lat = (i as f32 / rings as f32) * PI as f32;
        let sin_lat = lat.sin();
        let cos_lat = lat.cos();

        for j in 0..segments {
            let lng = (j as f32 / segments as f32) * 2.0 * PI as f32;
            let sin_lng = lng.sin();
            let cos_lng = lng.cos();

            let x = r * sin_lat * cos_lng;
            let y = r * cos_lat;
            let z = r * sin_lat * sin_lng;

            vertices.push(Point3::new(x, y, z));
        }
    }

    // Generate indices
    for i in 0..rings {
        for j in 0..segments {
            let next_j = (j + 1) % segments;

            let a = i * segments + j;
            let b = a + segments;
            let c = i * segments + next_j;
            let d = c + segments;

            if i > 0 {
                indices.push([a, b, c]);
            }
            if i < rings - 1 {
                indices.push([b, d, c]);
            }
        }
    }

    let normals = calculate_vertex_normals(&vertices, &indices);

    Ok(MeshData::new(vertices, indices, normals))
}

fn build_cone(params: &HashMap<String, f64>) -> Result<MeshData, GeometryError> {
    let radius = *params.get("radius").unwrap_or(&1.0);
    let height = *params.get("height").unwrap_or(&2.0);
    let segments = *params.get("segments").unwrap_or(&32.0) as usize;

    let mut vertices = Vec::new();
    let mut indices = Vec::new();

    let r = radius as f32;
    let h = height as f32;

    // Add apex vertex
    vertices.push(Point3::new(0.0, h / 2.0, 0.0)); // Apex at top
    let apex_idx = 0;

    // Add base circle vertices
    for i in 0..segments {
        let angle = (i as f32 / segments as f32) * 2.0 * PI as f32;
        let x = r * angle.cos();
        let z = r * angle.sin();
        vertices.push(Point3::new(x, -h / 2.0, z));
    }

    // Add base center
    vertices.push(Point3::new(0.0, -h / 2.0, 0.0));
    let base_center_idx = segments + 1;

    // Generate side faces
    for i in 0..segments {
        let next = (i + 1) % segments;
        let base1 = i + 1; // +1 because apex is at index 0
        let base2 = next + 1;

        // Side triangle
        indices.push([apex_idx, base2, base1]);

        // Base triangle
        indices.push([base_center_idx, base1, base2]);
    }

    let normals = calculate_vertex_normals(&vertices, &indices);

    Ok(MeshData::new(vertices, indices, normals))
}

fn build_torus(params: &HashMap<String, f64>) -> Result<MeshData, GeometryError> {
    let major_radius = *params.get("radius").unwrap_or(&1.5) as f32;
    let minor_radius = *params.get("tube").unwrap_or(&0.4) as f32;
    let major_segments = *params.get("majorSegments").unwrap_or(&32.0) as usize;
    let minor_segments = *params.get("minorSegments").unwrap_or(&16.0) as usize;

    // Generate torus mesh manually (parry3d doesn't have a built-in torus)
    let mut vertices = Vec::new();
    let mut indices = Vec::new();

    for i in 0..major_segments {
        let theta = (i as f32 / major_segments as f32) * 2.0 * PI as f32;
        let cos_theta = theta.cos();
        let sin_theta = theta.sin();

        for j in 0..minor_segments {
            let phi = (j as f32 / minor_segments as f32) * 2.0 * PI as f32;
            let cos_phi = phi.cos();
            let sin_phi = phi.sin();

            let x = (major_radius + minor_radius * cos_phi) * cos_theta;
            let y = minor_radius * sin_phi;
            let z = (major_radius + minor_radius * cos_phi) * sin_theta;

            vertices.push(Point3::new(x, y, z));
        }
    }

    // Generate indices
    for i in 0..major_segments {
        let next_i = (i + 1) % major_segments;
        for j in 0..minor_segments {
            let next_j = (j + 1) % minor_segments;

            let a = i * minor_segments + j;
            let b = next_i * minor_segments + j;
            let c = next_i * minor_segments + next_j;
            let d = i * minor_segments + next_j;

            // Two triangles per quad
            indices.push([a, b, c]);
            indices.push([a, c, d]);
        }
    }

    let normals = calculate_vertex_normals(&vertices, &indices);

    Ok(MeshData::new(vertices, indices, normals))
}

fn build_wedge(params: &HashMap<String, f64>) -> Result<MeshData, GeometryError> {
    let width = *params.get("width").unwrap_or(&2.0) as f32;
    let height = *params.get("height").unwrap_or(&2.0) as f32;
    let depth = *params.get("depth").unwrap_or(&2.0) as f32;

    let hw = width / 2.0;
    let hh = height / 2.0;
    let hd = depth / 2.0;

    // 6 vertices: triangle extruded along Z
    let vertices = vec![
        Point3::new(-hw, -hh, -hd), // 0 front bottom-left
        Point3::new(hw, -hh, -hd),  // 1 front bottom-right
        Point3::new(0.0, hh, -hd),  // 2 front apex
        Point3::new(-hw, -hh, hd),  // 3 back bottom-left
        Point3::new(hw, -hh, hd),   // 4 back bottom-right
        Point3::new(0.0, hh, hd),   // 5 back apex
    ];

    let indices = vec![
        [0, 2, 1], // front
        [3, 4, 5], // back
        [0, 1, 4],
        [0, 4, 3], // bottom
        [1, 2, 5],
        [1, 5, 4], // right slope
        [2, 0, 3],
        [2, 3, 5], // left slope
    ];

    let normals = calculate_vertex_normals(&vertices, &indices);

    Ok(MeshData::new(vertices, indices, normals))
}

fn build_tube(params: &HashMap<String, f64>) -> Result<MeshData, GeometryError> {
    let outer_radius = *params.get("radius").unwrap_or(&1.0) as f32;
    let inner_radius = *params.get("innerRadius").unwrap_or(&0.5) as f32;
    let height = *params.get("height").unwrap_or(&2.0) as f32;
    let segments = *params.get("segments").unwrap_or(&32.0) as usize;

    let mut vertices = Vec::new();
    let mut indices = Vec::new();

    let hh = height / 2.0;

    // Generate vertices for outer and inner cylinders
    for i in 0..segments {
        let angle = (i as f32 / segments as f32) * 2.0 * PI as f32;
        let cos_a = angle.cos();
        let sin_a = angle.sin();

        // Outer cylinder vertices (top and bottom)
        vertices.push(Point3::new(outer_radius * cos_a, hh, outer_radius * sin_a));
        vertices.push(Point3::new(outer_radius * cos_a, -hh, outer_radius * sin_a));

        // Inner cylinder vertices (top and bottom)
        vertices.push(Point3::new(inner_radius * cos_a, hh, inner_radius * sin_a));
        vertices.push(Point3::new(inner_radius * cos_a, -hh, inner_radius * sin_a));
    }

    // Generate faces
    for i in 0..segments {
        let next = (i + 1) % segments;

        let ot = i * 4; // outer top
        let ob = i * 4 + 1; // outer bottom
        let it = i * 4 + 2; // inner top
        let ib = i * 4 + 3; // inner bottom

        let not = next * 4;
        let nob = next * 4 + 1;
        let nit = next * 4 + 2;
        let nib = next * 4 + 3;

        // Outer wall
        indices.push([ot, ob, nob]);
        indices.push([ot, nob, not]);

        // Inner wall (reversed normals)
        indices.push([it, nib, ib]);
        indices.push([it, nit, nib]);

        // Top ring
        indices.push([ot, not, nit]);
        indices.push([ot, nit, it]);

        // Bottom ring
        indices.push([ob, ib, nib]);
        indices.push([ob, nib, nob]);
    }

    let normals = calculate_vertex_normals(&vertices, &indices);

    Ok(MeshData::new(vertices, indices, normals))
}

fn build_pyramid(params: &HashMap<String, f64>) -> Result<MeshData, GeometryError> {
    let width = *params.get("width").unwrap_or(&2.0) as f32;
    let depth = *params.get("depth").unwrap_or(&2.0) as f32;
    let height = *params.get("height").unwrap_or(&2.0) as f32;

    let hw = width / 2.0;
    let hd = depth / 2.0;

    let vertices = vec![
        // Base vertices
        Point3::new(-hw, 0.0, -hd), // 0
        Point3::new(hw, 0.0, -hd),  // 1
        Point3::new(hw, 0.0, hd),   // 2
        Point3::new(-hw, 0.0, hd),  // 3
        // Apex
        Point3::new(0.0, height, 0.0), // 4
    ];

    let indices = vec![
        // Base (looking up)
        [0, 2, 1],
        [0, 3, 2],
        // Sides
        [0, 1, 4], // Front
        [1, 2, 4], // Right
        [2, 3, 4], // Back
        [3, 0, 4], // Left
    ];

    let normals = calculate_vertex_normals(&vertices, &indices);

    Ok(MeshData::new(vertices, indices, normals))
}

fn build_ellipsoid(params: &HashMap<String, f64>) -> Result<MeshData, GeometryError> {
    let radius_x = *params.get("radiusX").unwrap_or(&1.0) as f32;
    let radius_y = *params.get("radiusY").unwrap_or(&0.8) as f32;
    let radius_z = *params.get("radiusZ").unwrap_or(&1.2) as f32;
    let segments = *params.get("segments").unwrap_or(&32.0) as usize;
    let rings = *params.get("rings").unwrap_or(&16.0) as usize;

    let mut vertices = Vec::new();
    let mut indices = Vec::new();

    // Generate vertices
    for i in 0..=rings {
        let lat = (i as f32 / rings as f32) * PI as f32;
        let sin_lat = lat.sin();
        let cos_lat = lat.cos();

        for j in 0..segments {
            let lng = (j as f32 / segments as f32) * 2.0 * PI as f32;
            let sin_lng = lng.sin();
            let cos_lng = lng.cos();

            let x = radius_x * sin_lat * cos_lng;
            let y = radius_y * cos_lat;
            let z = radius_z * sin_lat * sin_lng;

            vertices.push(Point3::new(x, y, z));
        }
    }

    // Generate indices
    for i in 0..rings {
        for j in 0..segments {
            let next_j = (j + 1) % segments;

            let a = i * segments + j;
            let b = a + segments;
            let c = i * segments + next_j;
            let d = c + segments;

            if i > 0 {
                indices.push([a, b, c]);
            }
            if i < rings - 1 {
                indices.push([b, d, c]);
            }
        }
    }

    let normals = calculate_vertex_normals(&vertices, &indices);

    Ok(MeshData::new(vertices, indices, normals))
}

// ─────────────────────────────────────────────────────────────────────────────
// Utility Functions
// ─────────────────────────────────────────────────────────────────────────────

pub fn calculate_vertex_normals(
    vertices: &[Point3<f32>],
    faces: &[[usize; 3]],
) -> Vec<Vector3<f32>> {
    let mut normals = vec![Vector3::zeros(); vertices.len()];

    // Calculate face normals and accumulate to vertex normals
    for face in faces {
        if let (Some(v0), Some(v1), Some(v2)) = (
            vertices.get(face[0]),
            vertices.get(face[1]),
            vertices.get(face[2]),
        ) {
            let edge1 = v1 - v0;
            let edge2 = v2 - v0;
            let cross = edge1.cross(&edge2);
            let len = cross.norm();
            if len <= 1e-8 {
                continue;
            }
            let face_normal = cross / len;

            normals[face[0]] += face_normal;
            normals[face[1]] += face_normal;
            normals[face[2]] += face_normal;
        }
    }

    // Normalize accumulated normals
    for normal in &mut normals {
        let len = normal.norm();
        if len <= 1e-8 {
            *normal = Vector3::new(0.0, 1.0, 0.0);
        } else {
            *normal /= len;
        }
    }

    normals
}

fn axis_index(axis: &str) -> Result<usize, GeometryError> {
    match axis.to_ascii_lowercase().as_str() {
        "x" => Ok(0),
        "y" => Ok(1),
        "z" => Ok(2),
        other => Err(GeometryError::InvalidParameters(format!(
            "Invalid axis '{}'. Expected x, y, or z",
            other
        ))),
    }
}

fn side_sign(side: &str) -> f32 {
    if side.eq_ignore_ascii_case("negative") {
        -1.0
    } else {
        1.0
    }
}

fn vertex_component(x: f32, y: f32, z: f32, axis_idx: usize) -> f32 {
    match axis_idx {
        0 => x,
        1 => y,
        _ => z,
    }
}

fn rotate_pair(a: f32, b: f32, angle_rad: f32) -> (f32, f32) {
    let c = angle_rad.cos();
    let s = angle_rad.sin();
    (a * c - b * s, a * s + b * c)
}

fn recompute_mesh_normals(mesh: &mut MeshData) -> Result<(), GeometryError> {
    if mesh.indices.len() % 3 != 0 {
        return Err(GeometryError::MeshGenerationFailed(
            "Index array length is not divisible by 3".to_string(),
        ));
    }

    let mut vertices = Vec::with_capacity(mesh.vertex_count());
    for i in 0..mesh.vertex_count() {
        vertices.push(Point3::new(
            mesh.vertices[i * 3],
            mesh.vertices[i * 3 + 1],
            mesh.vertices[i * 3 + 2],
        ));
    }

    let mut faces = Vec::with_capacity(mesh.face_count());
    for tri in mesh.indices.chunks(3) {
        faces.push([tri[0] as usize, tri[1] as usize, tri[2] as usize]);
    }

    let normals = calculate_vertex_normals(&vertices, &faces);
    mesh.normals.clear();
    mesh.normals.reserve(normals.len() * 3);
    for n in normals {
        mesh.normals.push(n.x);
        mesh.normals.push(n.y);
        mesh.normals.push(n.z);
    }

    Ok(())
}

fn rebuild_edges(mesh: &mut MeshData) {
    mesh.edges.clear();
    mesh.edges.reserve(mesh.indices.len() * 2);
    for tri in mesh.indices.chunks(3) {
        if tri.len() == 3 {
            mesh.edges
                .extend_from_slice(&[tri[0], tri[1], tri[1], tri[2], tri[2], tri[0]]);
        }
    }
}

fn mesh_centroid(mesh: &MeshData) -> Point3<f32> {
    let count = mesh.vertex_count().max(1) as f32;
    let mut x = 0.0f32;
    let mut y = 0.0f32;
    let mut z = 0.0f32;
    for i in 0..mesh.vertex_count() {
        x += mesh.vertices[i * 3];
        y += mesh.vertices[i * 3 + 1];
        z += mesh.vertices[i * 3 + 2];
    }
    Point3::new(x / count, y / count, z / count)
}

fn build_vertex_neighbors(indices: &[u32], vertex_count: usize) -> Vec<Vec<usize>> {
    let mut sets = vec![std::collections::BTreeSet::<usize>::new(); vertex_count];
    for tri in indices.chunks(3) {
        if tri.len() != 3 {
            continue;
        }
        let a = tri[0] as usize;
        let b = tri[1] as usize;
        let c = tri[2] as usize;
        if a >= vertex_count || b >= vertex_count || c >= vertex_count {
            continue;
        }
        sets[a].insert(b);
        sets[a].insert(c);
        sets[b].insert(a);
        sets[b].insert(c);
        sets[c].insert(a);
        sets[c].insert(b);
    }
    sets.into_iter().map(|s| s.into_iter().collect()).collect()
}

fn laplacian_smooth(mesh: &mut MeshData, neighbors: &[Vec<usize>], alpha: f32, passes: usize) {
    let mut next = mesh.vertices.clone();
    for _ in 0..passes {
        for i in 0..mesh.vertex_count() {
            let neigh = &neighbors[i];
            if neigh.is_empty() {
                continue;
            }
            let mut ax = 0.0f32;
            let mut ay = 0.0f32;
            let mut az = 0.0f32;
            for n in neigh {
                ax += mesh.vertices[n * 3];
                ay += mesh.vertices[n * 3 + 1];
                az += mesh.vertices[n * 3 + 2];
            }
            let inv = 1.0 / neigh.len() as f32;
            ax *= inv;
            ay *= inv;
            az *= inv;

            let px = mesh.vertices[i * 3];
            let py = mesh.vertices[i * 3 + 1];
            let pz = mesh.vertices[i * 3 + 2];
            next[i * 3] = px + (ax - px) * alpha;
            next[i * 3 + 1] = py + (ay - py) * alpha;
            next[i * 3 + 2] = pz + (az - pz) * alpha;
        }
        mesh.vertices.clone_from(&next);
    }
}

fn sample_mesh_vertices(mesh: &MeshData, samples: usize) -> Vec<Point3<f32>> {
    let count = mesh.vertex_count();
    if count == 0 || samples == 0 {
        return Vec::new();
    }
    let take = samples.min(count);
    let stride = (count as f32 / take as f32).max(1.0);
    let mut out = Vec::with_capacity(take);
    let mut idx = 0.0f32;
    for _ in 0..take {
        let i = idx.floor() as usize;
        let clamped = i.min(count - 1);
        out.push(Point3::new(
            mesh.vertices[clamped * 3],
            mesh.vertices[clamped * 3 + 1],
            mesh.vertices[clamped * 3 + 2],
        ));
        idx += stride;
    }
    out
}

fn apply_revolve_deformation(
    mesh: &mut MeshData,
    axis: &str,
    side: &str,
    distance: f32,
    angle_deg: f32,
) -> Result<(), GeometryError> {
    let axis_idx = axis_index(axis)?;
    let sign = side_sign(side);

    let mut min_v = f32::INFINITY;
    let mut max_v = f32::NEG_INFINITY;
    for i in 0..mesh.vertex_count() {
        let x = mesh.vertices[i * 3];
        let y = mesh.vertices[i * 3 + 1];
        let z = mesh.vertices[i * 3 + 2];
        let c = vertex_component(x, y, z, axis_idx);
        min_v = min_v.min(c);
        max_v = max_v.max(c);
    }

    let span = (max_v - min_v).max(1e-6);
    let total_angle = angle_deg.to_radians() * sign;
    let travel = distance * sign;

    for i in 0..mesh.vertex_count() {
        let x = mesh.vertices[i * 3];
        let y = mesh.vertices[i * 3 + 1];
        let z = mesh.vertices[i * 3 + 2];
        let axis_c = vertex_component(x, y, z, axis_idx);
        let t = ((axis_c - min_v) / span).clamp(0.0, 1.0);
        let theta = total_angle * t;

        let (nx, ny, nz) = match axis_idx {
            0 => {
                let (ry, rz) = rotate_pair(y, z, theta);
                (x + travel * t, ry, rz)
            }
            1 => {
                let (rx, rz) = rotate_pair(x, z, theta);
                (rx, y + travel * t, rz)
            }
            _ => {
                let (rx, ry) = rotate_pair(x, y, theta);
                (rx, ry, z + travel * t)
            }
        };

        mesh.vertices[i * 3] = nx;
        mesh.vertices[i * 3 + 1] = ny;
        mesh.vertices[i * 3 + 2] = nz;
    }

    recompute_mesh_normals(mesh)
}

fn apply_sweep_deformation(
    mesh: &mut MeshData,
    axis: &str,
    side: &str,
    distance: f32,
    start_angle_deg: f32,
    end_angle_deg: f32,
    path_rotate_deg: f32,
    twist_deg: f32,
) -> Result<(), GeometryError> {
    let axis_idx = axis_index(axis)?;
    let sign = side_sign(side);

    let mut min_v = f32::INFINITY;
    let mut max_v = f32::NEG_INFINITY;
    for i in 0..mesh.vertex_count() {
        let x = mesh.vertices[i * 3];
        let y = mesh.vertices[i * 3 + 1];
        let z = mesh.vertices[i * 3 + 2];
        let c = vertex_component(x, y, z, axis_idx);
        min_v = min_v.min(c);
        max_v = max_v.max(c);
    }

    let span = (max_v - min_v).max(1e-6);
    let start = start_angle_deg.to_radians();
    let end = end_angle_deg.to_radians();
    let path_rot = path_rotate_deg.to_radians();
    let twist = twist_deg.to_radians();
    let travel = distance * sign;

    for i in 0..mesh.vertex_count() {
        let x = mesh.vertices[i * 3];
        let y = mesh.vertices[i * 3 + 1];
        let z = mesh.vertices[i * 3 + 2];
        let axis_c = vertex_component(x, y, z, axis_idx);
        let t = ((axis_c - min_v) / span).clamp(0.0, 1.0);

        let yaw = start + (end - start) * t + path_rot;
        let local_twist = twist * t;
        let offset = travel * t;

        let (mut nx, mut ny, mut nz) = (x, y, z);
        match axis_idx {
            0 => {
                ny += yaw.sin() * offset;
                nz += yaw.cos() * offset;
                let (ry, rz) = rotate_pair(ny, nz, local_twist);
                ny = ry;
                nz = rz;
            }
            1 => {
                nx += yaw.sin() * offset;
                nz += yaw.cos() * offset;
                let (rx, rz) = rotate_pair(nx, nz, local_twist);
                nx = rx;
                nz = rz;
            }
            _ => {
                nx += yaw.sin() * offset;
                ny += yaw.cos() * offset;
                let (rx, ry) = rotate_pair(nx, ny, local_twist);
                nx = rx;
                ny = ry;
            }
        }

        mesh.vertices[i * 3] = nx;
        mesh.vertices[i * 3 + 1] = ny;
        mesh.vertices[i * 3 + 2] = nz;
    }

    recompute_mesh_normals(mesh)
}
