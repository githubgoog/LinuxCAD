# LinuxCAD

**Version 0.3.0** — Now with blazing-fast Rust backend! 🚀

A Linux-first, cross-platform parametric CAD application modelled after the simplicity of Onshape, built with modern web technologies.

## What's New in 0.3.0

- ⚡ **100% Rust Backend** — Migrated from Python to Rust for 10-100x performance improvements
- 🔧 **53 Engineering Templates** — Expanded from 17 to 53 professional templates across 11 categories
- 🎯 **Enhanced Measurement Tools** — Smart snapping, multiple measurement types, real-time feedback
- 📐 **Enhanced Viewport Navigation** — Camera presets, fit-to-model, smooth animations
- 💡 **Contextual Help System** — Mode-aware tips and inline guidance
- 📊 **Performance Monitor** — Real-time FPS tracking with optimization suggestions
- ♿ **Accessibility Improvements** — UI scaling, high contrast, reduced motion support
- 🔄 **Undo/Redo** — Full 50-step history for all operations
- ⌨️ **Command Palette** — Searchable commands with Ctrl+K
- 🏗️ **Build Health Checks** — Pre-build validation for smooth packaging

## Stack

| Layer | Technology |
|-------|-----------|
| Desktop Shell | Electron 41 |
| UI / Shell | React 18 + TypeScript |
| 3D Rendering | Three.js via React Three Fiber + Drei |
| State | Zustand |
| Build | Vite 5 |
| Backend (Geometry/CAD) | Rust + Axum + high-performance solvers |

## Prerequisites

You need **Node.js ≥ 18** and **Rust** (latest stable) on your system.

### Install Node.js (if not installed)

```bash
# Option A — Linux package manager
sudo apt install nodejs npm       # Debian / Ubuntu / Mint
sudo dnf install nodejs npm       # Fedora / RHEL
sudo pacman -S nodejs npm         # Arch / Manjaro

# Option B — macOS (Homebrew)
brew install node

# Option C — Windows (winget)
winget install OpenJS.NodeJS.LTS

# Option D — via nvm (recommended)
curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.39.7/install.sh | bash
# restart your terminal, then:
nvm install --lts
```

### Install Rust (if not installed)

```bash
# All platforms — rustup (recommended)
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
source ~/.cargo/env  # or restart your terminal

# Windows (alternative via winget)
winget install Rustlang.Rustup

# macOS (alternative via Homebrew)
brew install rust

# After installation, verify:
cargo --version
rustc --version
```

> **Note:** If you use VS Code installed as a **Flatpak**, open a regular system terminal (e.g. GNOME Terminal) for the commands below, as the Flatpak sandbox may not see `node`/`npm`.

## Quick Start

### Linux

Open a **regular terminal** (not the VS Code integrated terminal if using Flatpak VS Code):

```bash
cd ~/Coding/"Linux CAD"
chmod +x run
./run
```

This starts LinuxCAD in the Electron desktop shell.

To update from the latest downloadable release artifact via terminal:

```bash
cd ~/Coding/"Linux CAD"
./update --release --asset appimage
```

Inside the desktop app, you can also check for new versions from:

- Help -> Check for Updates
- Menu bar More actions -> Check for Updates

For source-based update + rebuild instead:

```bash
./update
```

### macOS

Use the cross-platform npm scripts from `frontend/`:

```bash
cd frontend
npm install
npm run dev
```

In a second terminal:

```bash
cd frontend
npm run electron:dev
```

### Windows (PowerShell)

Desktop launcher:

```powershell
cd "C:\path\to\Linux CAD"
powershell -ExecutionPolicy Bypass -File .\run.ps1
```

Full backend + frontend launcher:

```powershell
cd "C:\path\to\Linux CAD"
powershell -ExecutionPolicy Bypass -File .\start.ps1
```

> In the Electron desktop app, `.lcad` projects are stored as filesystem-backed
> project folders (manifest + per-feature files) instead of a single raw JSON
> blob. Existing legacy `.lcad` JSON files can still be opened.

> The Rust backend runs automatically when you launch the Electron app. All CAD operations including boolean ops, sketching, and constraints are handled by the high-performance Rust backend.

## Manual Start

**Desktop app (Electron):**
```bash
cd frontend
npm install
npm run dev
npm run electron:dev
```

**Desktop app (Electron, safe mode / fallback GPU path):**
```bash
cd frontend
npm run dev
npm run electron:dev:safe
```

**Frontend web-only development:**
```bash
cd frontend
npm install
npm run dev
```

**Backend (Rust - auto-starts with Electron, manual start if needed):**
```bash
cd backend-rust
cargo build --release
cargo run --release
```

Development mode with hot-reload:
```bash
cd backend-rust
cargo install cargo-watch
cargo watch -x run
```

## Packaging

Use platform-aware Electron packaging scripts from `frontend/`:

```bash
cd frontend
npm install
npm run electron:build
```

Platform-specific packaging scripts:

- Linux (`AppImage` + `.deb`): `npm run electron:build:linux`
- macOS (`dmg` + `zip`): `npm run electron:build:mac`
- Windows (`nsis` + `zip`): `npm run electron:build:win`
- All targets (CI): `npm run electron:build:all`

Automated release publishing is available through GitHub Actions:

- Tag push like `v0.3.1` triggers [.github/workflows/release-publish.yml](.github/workflows/release-publish.yml)
- The workflow builds Linux/macOS/Windows packages and attaches assets to the GitHub Release

## Import External Models

LinuxCAD can import external mesh model files directly through the GUI:

**Using Electron app:**
1. Go to **File → Import External Model** (or press `Ctrl+Shift+I`)
2. Select your model file (STEP, IGES, STL, OBJ, PLY, etc.)
3. The Rust backend will automatically convert and import it as a baked mesh feature

**Supported formats:**
- **Mesh formats:** STL, OBJ, PLY, GLB, GLTF, OFF, 3MF
- **CAD formats:** STEP (.step, .stp), IGES (.iges, .igs)

Notes:
- STEP/IGES files are converted to mesh geometry (not parametric features)
- Import scale and merge settings can be configured in preferences
- Large models may take a few seconds to process

## Claude Desktop Integration (MCP)

> **Note:** Claude Desktop does not currently have a Linux release.
> This integration is ready for when you use LinuxCAD on **Windows and macOS**.

LinuxCAD includes an **MCP (Model Context Protocol) server** that lets Claude Desktop
act as a CAD agent — you can ask Claude to design and build models for you in natural language.

> The MCP server is a standalone Python tool that reads/writes `.lcad` files directly and does not depend on the Rust or Python backends.

### 1. Install Python and MCP dependency

```bash
# Install Python 3.10+ if not already installed
python3 --version

# Install MCP
pip install "mcp>=1.0.0"
# or use a virtual environment
python3 -m venv ~/.linuxcad-mcp
source ~/.linuxcad-mcp/bin/activate
pip install "mcp>=1.0.0"
```

### 2. Configure Claude Desktop

Open (or create) the Claude Desktop config file:

```
~/.config/Claude/claude_desktop_config.json
```

Add the LinuxCAD server entry — adjust the path to wherever you cloned the repo:

```json
{
  "mcpServers": {
    "linuxcad": {
      "command": "python3",
      "args": ["/home/<your-username>/Coding/Linux CAD/backend/mcp_server.py"]
    }
  }
}
```

> If your Python binary or `mcp` package is inside a virtual environment, use the
> full path to that environment's interpreter instead, e.g.
> `"/home/<user>/Coding/Linux CAD/backend/.venv/bin/python"`.

### 3. Restart Claude Desktop

After saving the config, quit and relaunch Claude Desktop.
You should see a 🔧 hammer icon in the chat bar confirming the MCP server connected.

### 4. Example prompts

Ask Claude things like:

- *"Create a simple bracket: a 40×40×5 mm base plate with a 40×30×5 mm side wall perpendicular to it."*
- *"Build a shaft assembly — 80 mm long, 10 mm radius cylinder, with a 20 mm long, 15 mm radius flange at one end."*
- *"Make a pipe elbow connecting two 25 mm tubes at 90 degrees, then save it to ~/parts/elbow.lcad"*
- *"Load ~/parts/bracket.lcad and add four M5 bolt holes in the corners."*

### Available MCP Tools

| Tool | Description |
|------|-------------|
| `scene_info` | Summary of the current scene |
| `list_feature_types` | All available shapes and their parameters |
| `add_feature` | Add any primitive, mechanical part, or operation (supports optional `description` field) |
| `modify_feature` | Change name, colour, visibility, or any param |
| `move_feature` | Set position / rotation of a feature |
| `delete_feature` | Remove a feature by ID |
| `clear_scene` | Start fresh |
| `set_scene_metadata` | Set model name and units |
| `save_model` | Save to a `.lcad` file |
| `load_model` | Load a `.lcad` file |
| `new_model` | Start a new empty model |
| `get_feature_details` | Inspect all details of a specific feature (including its description) |
| `export_model_summary` | Get a human-readable summary of the entire model with all descriptions |

### Editing Claude-Generated Models

When Claude creates a model and saves it, here's how to edit it:

**1. Ask Claude to describe what it made:**
   - Claude can call `export_model_summary` to tell you exactly what features are in the model and why each one exists

**2. Open the saved `.lcad` file in LinuxCAD:**
   - Launch LinuxCAD
   - Go to **File → Open** and select the `.lcad` file Claude saved
   - Or drag-and-drop the file onto the LinuxCAD viewport

**3. Edit features in the GUI:**
   - Click any feature in the **Feature Tree** (left panel) to select it
   - Edit its parameters in the **Property Panel** (right panel):
     - Drag sliders to adjust dimensions
     - Change position (posX, posY, posZ)
     - Change rotation angles
     - Change colour
   - View changes in real-time in the 3D viewport

**4. Save your edits:**
   - Press **Ctrl+S** (or **File → Save**) to overwrite the `.lcad` file

**Example workflow:**
```
You: "Create a simple rectangular bracket: 40×40×30 mm base, 5 mm wall thickness, and a single mounting hole."

Claude: [creates 3 features with descriptions, saves to ~/parts/bracket.lcad]
         "Created a bracket assembly at ~/parts/bracket.lcad:
          1. Base Plate (40×40×5mm) — primary mounting surface
          2. Back Wall (40×5×30mm) — vertical support
          3. Hole (Ø10mm) — central mounting hole"

You: [Open ~/parts/bracket.lcad in LinuxCAD]
     [Want the hole bigger? Click the Hole feature, change radius to 15mm]
     [Want to move the wall higher? Adjust that feature's posY]
     [Ctrl+S to save]
```

Opening a Claude-generated model in LinuxCAD

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| `Ctrl+1..9` | Add primitives (Box..Ellipsoid) |
| `W` | Move gizmo |
| `R` | Rotate gizmo |
| `S` | Scale gizmo |
| `E` | Extrude mode |
| `C` | Chamfer mode |
| `F` | Fillet mode |
| `K` | Cut mode |
| `J` | Hole tool mode |
| `G` | Move to face center |
| `M` | Mirror |
| `L` | Linear array |
| `O` | Polar array |
| `B` | Boolean mode |
| `H` | Shell / hollow |
| `X` | Toggle snapping |
| `Esc` | Deselect / back to Select tool |
| `Delete` / `Backspace` | Delete selected feature |

## Features

- [x] Dark CAD-style UI (inspired by Onshape / Fusion 360)
- [x] 3D viewport: orbit, pan, zoom, view cube
- [x] Infinite reference grid
- [x] Add primitives: Box, Cylinder, Sphere, Cone, Torus, Wedge, Tube, Pyramid, Ellipsoid
- [x] Feature tree with show/hide, rename, delete
- [x] Property panel: live-edit dimensions, position, rotation, colour
- [x] Keyboard shortcuts
- [x] **Rust backend** for high-performance CAD operations
- [x] Hole tool (through/blind with axis + offsets)
- [x] Mechanical analysis panel (surface area, volume, center of mass)
- [x] Interference check (AABB overlap + precise)
- [x] **Boolean operations** (Union, Subtract, Intersect)
- [x] **Fillet / Chamfer** edge operations
- [x] **Pattern operations** (Linear Array, Polar Array)
- [x] **Shell / Hollow** operation
- [x] **Mirror** operation
- [x] **Undo / Redo** with 50-step history
- [x] **Command palette** (Ctrl+K) with searchable commands
- [x] **53 engineering templates** across 11 categories
- [x] **Enhanced measurement tools** with smart snapping
- [x] **Contextual help system** with mode-aware tips
- [x] **Performance monitor** with FPS tracking and optimization suggestions
- [x] **Enhanced viewport navigation** with camera presets
- [x] **Accessibility features** (UI scaling, high contrast, reduced motion)

## Roadmap

### Phase 2 – Sketch-based Modelling (In Progress)
- [x] Sketch mode on XY / XZ / YZ planes
- [x] Line, circle, arc, rectangle primitives
- [x] Extrude from sketch
- [x] Revolve from sketch
- [ ] Sweep and Loft from sketch
- [ ] Parametric constraints (distance, angle, parallel, perpendicular)

### Phase 3 – File Management
- [x] Save / load `.lcad` projects
- [x] Export STL
- [x] Import STEP, IGES, STL, OBJ, PLY, GLB, OFF, 3MF
- [ ] Export STEP (parametric)
- [ ] Cloud sync and collaboration

### Phase 4 – Advanced Analysis
- [ ] Finite Element Analysis (FEA)
- [ ] Stress simulation
- [ ] Mass properties with materials
- [ ] Sheet metal unfolding
- [ ] CAM toolpath generation

### Phase 5 – Polish & UX
- [x] Measurement tools
- [ ] Dimension annotations
- [x] Undo / Redo history
- [ ] Dark/light theme toggle
- [x] Keyboard shortcuts
- [x] Tutorial system
- [x] Accessibility improvements

## Project Structure

```
Linux CAD/
├── run                       # Main Electron launcher
├── run.ps1                   # Windows Electron launcher
├── start.sh                  # Web + backend dev launcher
├── start.ps1                 # Windows web + backend dev launcher
├── frontend/
│   ├── src/
│   │   ├── App.tsx           # Root layout with error boundary
│   │   ├── index.css         # Global dark theme styles
│   │   ├── types/cad.ts      # Shared TypeScript types
│   │   ├── store/cadStore.ts # Zustand global state with undo/redo
│   │   ├── utils/            # Utilities (units, geometry, file ops)
│   │   └── components/
│   │       ├── MenuBar/      # Top menu bar + Rust backend status
│   │       ├── Toolbar/      # Shape tools and operations
│   │       ├── FeatureTree/  # Left hierarchical feature tree
│   │       ├── Viewport3D/   # Three.js 3D canvas with enhanced UX
│   │       ├── PropertyPanel/# Right property editor
│   │       ├── CommandPalette/# Ctrl+K command search
│   │       ├── TemplateLibrary/# 53 engineering templates
│   │       ├── SketchMode/   # 2D parametric sketching
│   │       └── HelpModal/    # Tutorial and help system
│   ├── electron/             # Electron main/preload and desktop assets
│   ├── scripts/              # Build automation and health checks
│   └── package.json          # v0.3.0 with Electron/Rust build config
├── backend-rust/             # High-performance Rust backend
│   ├── src/
│   │   ├── main.rs           # Axum web server (port 8000)
│   │   ├── geometry.rs       # Mesh generation and operations
│   │   ├── boolean_ops.rs    # CSG boolean operations
│   │   ├── constraint.rs     # Parametric constraint solver
│   │   ├── sketch.rs         # 2D sketch engine
│   │   └── import_export.rs  # STEP, STL, OBJ, PLY file I/O
│   ├── Cargo.toml            # Rust dependencies
│   ├── build.sh              # Production build script
│   └── README.md             # Backend documentation
└── backend/                  # Legacy Python tools (MCP server only)
    └── mcp_server.py         # Claude Desktop integration
```
