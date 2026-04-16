// Public modules
pub mod benchmarks;
pub mod boolean;
pub mod constraint;
pub mod geometry;
pub mod import_export;
pub mod sketch;
pub mod types;

// Re-export commonly used types
pub use boolean::BooleanEngine;
pub use constraint::ConstraintSolver;
pub use geometry::GeometryEngine;
pub use sketch::SketchEngine;
pub use types::*;

#[cfg(test)]
mod tests {
    use super::*;
    use serde_json::json;

    #[test]
    fn test_constraint_solver_basic() {
        // Test basic constraint solving functionality
        let mut solver = ConstraintSolver::new();

        // Add a simple variable
        solver.add_variable(Variable {
            id: "x".to_string(),
            value: 0.0,
            min_value: None,
            max_value: None,
            fixed: false,
        });

        // The system should solve even with no constraints
        let result = solver.solve().unwrap();
        assert!(result.success);
        assert_eq!(result.variables.len(), 1);
        assert!(result.variables.contains_key("x"));
    }

    #[test]
    fn test_constraint_solver_distance() {
        let mut solver = ConstraintSolver::new();

        // Add variables for two points
        solver.add_variable(Variable {
            id: "p1.x".to_string(),
            value: 0.0,
            min_value: None,
            max_value: None,
            fixed: false,
        });
        solver.add_variable(Variable {
            id: "p1.y".to_string(),
            value: 0.0,
            min_value: None,
            max_value: None,
            fixed: false,
        });
        solver.add_variable(Variable {
            id: "p2.x".to_string(),
            value: 1.0,
            min_value: None,
            max_value: None,
            fixed: false,
        });
        solver.add_variable(Variable {
            id: "p2.y".to_string(),
            value: 0.0,
            min_value: None,
            max_value: None,
            fixed: false,
        });

        // Add distance constraint
        solver.add_constraint(SolverConstraint {
            id: "distance1".to_string(),
            constraint_type: "distance".to_string(),
            variables: vec![
                "p1.x".to_string(),
                "p1.y".to_string(),
                "p2.x".to_string(),
                "p2.y".to_string(),
            ],
            target_value: Some(2.0), // Target distance of 2
            weight: 1.0,
        });

        let result = solver.solve().unwrap();
        assert!(result.success || result.residual < 0.1); // Allow some tolerance for convergence
    }

    #[test]
    fn test_sketch_engine_creation() {
        let engine = SketchEngine::new();

        // Create two points
        let p1 = SketchPoint::new(0.0, 0.0);
        let p2 = SketchPoint::new(1.0, 0.0);

        // Create a line
        let (points, entity) = engine.create_line(p1, p2);

        assert_eq!(points.len(), 2);
        match entity {
            SketchEntity::Line(line) => {
                assert_eq!(line.start_point_id, p1.id);
                assert_eq!(line.end_point_id, p2.id);
                assert!(line.visible);
                assert!(!line.construction);
            }
            _ => panic!("Expected line entity"),
        }
    }

    #[test]
    fn test_sketch_validation() {
        let engine = SketchEngine::new();

        // Create a simple valid sketch
        let mut sketch = Sketch {
            id: uuid::Uuid::new_v4(),
            name: "Test Sketch".to_string(),
            points: vec![SketchPoint::new(0.0, 0.0), SketchPoint::new(1.0, 0.0)],
            entities: Vec::new(),
            constraints: Vec::new(),
            visible: true,
            active: true,
            fully_constrained: false,
        };

        let validation = engine.validate_sketch(&sketch);
        assert!(validation.valid);
        assert!(validation.errors.is_empty());
    }

    #[test]
    fn test_mesh_data_serialization_has_compatibility_fields() {
        let mesh = MeshData {
            vertices: vec![0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0],
            indices: vec![0, 1, 2],
            normals: vec![0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0],
            edges: vec![0, 1, 1, 2, 2, 0],
        };

        let value = serde_json::to_value(&mesh).expect("MeshData should serialize");

        assert_eq!(value["vertices"], json!(mesh.vertices));
        assert_eq!(value["indices"], json!(mesh.indices));
        assert_eq!(value["normals"], json!(mesh.normals));
        assert_eq!(value["pos"], json!(mesh.vertices));
        assert_eq!(value["idx"], json!(mesh.indices));
        assert_eq!(value["norm"], json!(mesh.normals));
    }

    #[test]
    fn test_mesh_data_deserialization_supports_legacy_names() {
        let value = json!({
            "pos": [0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0],
            "idx": [0, 1, 2],
            "norm": [0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0]
        });

        let mesh: MeshData =
            serde_json::from_value(value).expect("Legacy mesh format should deserialize");
        assert_eq!(mesh.vertex_count(), 3);
        assert_eq!(mesh.face_count(), 1);
        assert_eq!(mesh.normals.len(), mesh.vertices.len());
    }
}

// Integration tests
#[cfg(test)]
mod integration_tests {
    use super::*;
    use std::collections::HashMap;

    #[test]
    fn test_geometry_engine_box() {
        let engine = GeometryEngine::new();

        let mut params = HashMap::new();
        params.insert("width".to_string(), 2.0);
        params.insert("height".to_string(), 3.0);
        params.insert("depth".to_string(), 4.0);

        let feature = Feature {
            id: "test-box".to_string(),
            feature_type: FeatureType::Primitive(PrimitiveType::Box),
            name: "Test Box".to_string(),
            assembly_id: None,
            visible: true,
            color: "#ff0000".to_string(),
            params,
            source_ids: None,
            baked_geometry: None,
        };

        let result = engine.compute_mesh(&feature);
        assert!(result.is_ok());

        let mesh = result.unwrap();
        assert!(mesh.vertex_count() > 0);
        assert!(mesh.face_count() > 0);
        assert_eq!(mesh.vertices.len() % 3, 0); // Vertices should be xyz triplets
        assert_eq!(mesh.indices.len() % 3, 0); // Indices should be triangles
        assert_eq!(mesh.normals.len(), mesh.vertices.len()); // One normal per vertex
    }

    #[test]
    fn test_geometry_engine_cylinder() {
        let engine = GeometryEngine::new();

        let mut params = HashMap::new();
        params.insert("radius".to_string(), 1.5);
        params.insert("height".to_string(), 3.0);

        let feature = Feature {
            id: "test-cylinder".to_string(),
            feature_type: FeatureType::Primitive(PrimitiveType::Cylinder),
            name: "Test Cylinder".to_string(),
            assembly_id: None,
            visible: true,
            color: "#00ff00".to_string(),
            params,
            source_ids: None,
            baked_geometry: None,
        };

        let result = engine.compute_mesh(&feature);
        assert!(result.is_ok());

        let mesh = result.unwrap();
        assert!(mesh.vertex_count() > 8); // Should have reasonable vertex count
        assert!(mesh.face_count() > 12); // Should have reasonable face count
    }

    #[test]
    fn test_end_to_end_constraint_solving() {
        // Create a sketch with two points and a distance constraint
        let point1 = SketchPoint::new(0.0, 0.0);
        let point2 = SketchPoint::new(3.0, 4.0); // Distance should be 5.0

        let distance_constraint = GeometricConstraint {
            id: uuid::Uuid::new_v4(),
            constraint_type: ConstraintType::Distance,
            entity_ids: Vec::new(),
            point_ids: Some(vec![point1.id, point2.id]),
            value: Some(10.0), // Target distance of 10
            enabled: true,
            satisfied: false,
        };

        let mut sketch = Sketch {
            id: uuid::Uuid::new_v4(),
            name: "Test Distance Constraint".to_string(),
            points: vec![point1, point2],
            entities: Vec::new(),
            constraints: vec![distance_constraint],
            visible: true,
            active: true,
            fully_constrained: false,
        };

        // Solve constraints
        let engine = SketchEngine::new();
        let result = engine.solve_constraints(&mut sketch);

        // The solver should converge (might not be exact due to local minima)
        assert!(result.is_ok());
        let solver_result = result.unwrap();

        // Check that the solver made progress
        assert!(solver_result.iterations > 0);

        // The residual should be reasonably small if the solver converged
        if solver_result.success {
            assert!(solver_result.residual < 1.0);
        }
    }
}

// Benchmark tests (requires nightly Rust)
#[cfg(all(feature = "unstable", test))]
mod benchmarks {
    use super::*;
    use std::collections::HashMap;
    extern crate test;
    use test::Bencher;

    #[bench]
    fn bench_constraint_solver_10_vars(b: &mut Bencher) {
        let mut solver = ConstraintSolver::new();

        // Add 10 variables
        for i in 0..10 {
            solver.add_variable(Variable {
                id: format!("var{}", i),
                value: i as f64,
                min_value: None,
                max_value: None,
                fixed: false,
            });
        }

        b.iter(|| solver.solve().unwrap());
    }

    #[bench]
    fn bench_box_generation(b: &mut Bencher) {
        let engine = GeometryEngine::new();
        let mut params = HashMap::new();
        params.insert("width".to_string(), 2.0);
        params.insert("height".to_string(), 2.0);
        params.insert("depth".to_string(), 2.0);

        let feature = Feature {
            id: "bench-box".to_string(),
            feature_type: FeatureType::Primitive(PrimitiveType::Box),
            name: "Benchmark Box".to_string(),
            assembly_id: None,
            visible: true,
            color: "#ffffff".to_string(),
            params,
            source_ids: None,
            baked_geometry: None,
        };

        b.iter(|| engine.compute_mesh(&feature).unwrap());
    }

    #[bench]
    fn bench_cylinder_generation(b: &mut Bencher) {
        let engine = GeometryEngine::new();
        let mut params = HashMap::new();
        params.insert("radius".to_string(), 1.0);
        params.insert("height".to_string(), 2.0);

        let feature = Feature {
            id: "bench-cylinder".to_string(),
            feature_type: FeatureType::Primitive(PrimitiveType::Cylinder),
            name: "Benchmark Cylinder".to_string(),
            assembly_id: None,
            visible: true,
            color: "#ffffff".to_string(),
            params,
            source_ids: None,
            baked_geometry: None,
        };

        b.iter(|| engine.compute_mesh(&feature).unwrap());
    }
}
