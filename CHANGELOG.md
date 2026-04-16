# Changelog

All notable changes to LinuxCAD will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.3.0] - 2026-03-25

### Added
- **Rust Backend Migration** — Complete migration from Python to Rust backend
  - 10-100x performance improvements for constraint solving and geometry operations
  - Axum web framework for high-performance HTTP serving
  - Boolean operations fully implemented in Rust
  - Model import/export through Rust API endpoints
- **Engineering Templates** — Expanded template library from 17 to 53 templates
  - Electronics: PCB Standoff, Heat Sink, Fan Mount, Enclosure Box, Panel Button, Display Mount
  - Robotics: Servo Mount, Motor Bracket, Wheel Assembly, Robotic Arm, Sensor Bracket, Gripper Jaw
  - Consumer Products: Knob, Handle, Phone Case, Container Lid, Snap-Fit, Living Hinge
  - Hardware: Hinge, Latch, Spring Clip, Threaded Insert, Quick Release, Cable Clip
  - Structural: T-Slot Extrusion, Angle Iron, I-Beam, Corner Brace, Frame Joint, Foot Pad
  - Tooling: Drill Guide, Fixture, Jig, Clamp, Vise Jaw, Positioning Pin
- **Enhanced Measurement Tools** — Smart measurement system with multiple types
  - Distance, angle, area, volume, and diameter measurements
  - Smart snapping to vertices, edges, and grid points
  - Real-time measurement preview
  - Measurement history display
- **Enhanced Viewport Navigation** — Professional camera control system
  - Pre-defined camera presets (isometric, front, right, top, bottom)
  - Fit-to-model automatic camera positioning
  - Smooth animated camera transitions
  - Smart camera positioning based on model bounds
- **Contextual Help System** — Mode-aware assistance
  - Context-sensitive help tips for each tool and mode
  - Keyboard shortcut reminders
  - Warning messages for missing selections
  - Dismissible and navigable tip system
- **Performance Monitor** — Real-time performance tracking
  - FPS monitoring and display
  - Memory usage tracking (when available)
  - Performance alerts for low FPS or high memory
  - Optimization suggestions for slow operations
  - Expandable detailed metrics panel
- **Build System Improvements** — Enhanced Electron packaging
  - Pre-build health check script with 8 validation checks
  - Automatic Rust backend compilation in all build commands
  - Cross-platform build scripts (Linux, macOS, Windows)
  - Build validation for Node.js, Rust, frontend assets, and backend binaries
- **Code Quality & Performance** — Major optimization pass
  - Eliminated critical code duplications
  - Optimized viewport components with memoization
  - Parallelized file I/O operations using Promise.all
  - Consolidated shared implementations

### Changed
- **Prerequisites** — Updated from Python 3.10+ to Rust (latest stable)
- **Backend** — All Python backend dependencies removed
- **Model Import** — Model conversion now uses Rust API endpoints instead of Python scripts
- **Electron** — Updated to Electron 41
- **File Operations** — Parallelized project file reads for faster loading
- **Documentation** — Updated README with current stack and features

### Removed
- Python backend virtual environment and dependencies
- Python model converter scripts
- Python-based geometry operations

### Fixed
- Code duplication issues in normalToRotDeg and calculate_vertex_normals
- Sequential file reads creating performance bottlenecks
- Missing type annotations and inconsistent implementations

## [0.2.15] - 2026-03-24

### Added
- Boolean operations (Union, Subtract, Intersect)
- Fillet and Chamfer edge operations
- Shell/Hollow operation
- Mirror operation
- Linear and Polar Array patterns
- Hole tool with through/blind options
- Mechanical analysis panel
- Interference checking
- Section view controls
- Sketch mode with 2D primitives
- Command palette (Ctrl+K)
- Undo/Redo functionality
- Template library with 17 templates
- Accessibility preferences system
- Tutorial modal

### Changed
- Improved viewport performance
- Enhanced feature tree functionality
- Better property panel organization

## [0.2.0] - 2026-03-10

### Added
- Electron desktop application shell
- Project file system (.lcad folder format)
- Feature tree with visibility toggles
- Property panel for live editing
- Basic primitives (Box, Cylinder, Sphere, Cone, Torus)
- 3D viewport with Three.js
- Dark CAD-style UI
- Keyboard shortcuts

### Changed
- Migrated from web-only to Electron desktop app
- Changed from single JSON to folder-based projects

## [0.1.0] - 2026-02-28

### Added
- Initial release
- React + TypeScript frontend
- Zustand state management
- Three.js 3D rendering
- Basic CAD primitives
- Python backend with FastAPI
