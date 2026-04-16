# LinuxCAD Build Instructions

LinuxCAD is a cross-platform desktop CAD application built with Electron and a Rust backend for high-performance geometric computations. This guide provides complete instructions for building and packaging the application for Linux, macOS, and Windows.

## Prerequisites

### Required Software

1. **Node.js** (version 20 or later)
   - Download from https://nodejs.org/
   - Or use a version manager like nvm/fnm

2. **Rust** (stable toolchain)
   - Install from https://rustup.rs/
   - Verify installation: `cargo --version`

3. **Git** (for cloning the repository)

### Platform-Specific Requirements

#### Linux
- GCC/Clang compiler toolchain
- On Ubuntu/Debian: `sudo apt install build-essential`
- For .deb package building: `dpkg-deb` (usually pre-installed)

#### macOS
- Xcode Command Line Tools: `xcode-select --install`
- For code signing (optional): Apple Developer account

#### Windows
- Visual Studio Build Tools or Visual Studio Community
- Windows SDK
- For NSIS installer: Install NSIS separately if needed

## Quick Start

### Update From Terminal

LinuxCAD can be updated directly from the terminal at the repository root:

```bash
./update
```

To update and launch immediately:

```bash
./update --launch
```

To update from the latest downloadable release artifact instead of rebuilding from source:

```bash
./update --release
```

Select release asset type explicitly:

```bash
# Install/update portable AppImage under ~/.local/bin/LinuxCAD.AppImage
./update --release --asset appimage

# Download latest .deb into frontend/release/
./update --release --asset deb

# Download + install latest .deb
./update --release --asset deb --install-deb
```

### Desktop Launch Modes

From the repository root:

```bash
./run
```

This uses hardware acceleration by default for best performance.

For fallback software rendering only when needed:

```bash
./run --safe
```

The convenience launcher supports the same behavior:

```bash
./start.sh
./start.sh --safe
```

### Clone and Setup

```bash
# Clone the repository
git clone <repository-url>
cd "Linux CAD"

# Install frontend dependencies
cd frontend
npm install

# Build and run in development mode
npm run electron:dev
```

## Building the Application

### Development Build

For development with hot-reloading:

```bash
cd frontend
npm run electron:dev
```

The Rust backend will be built automatically if not present, or you can build it manually:

```bash
# Build Rust backend manually
npm run build:rust

# Or build directly
cd ../backend-rust
cargo build --release
```

### Production Builds

#### Build for Current Platform

```bash
cd frontend
npm run electron:build
```

This will:
1. Build the Rust backend in release mode
2. Build the frontend TypeScript/React code
3. Package the Electron app for your current platform

#### Platform-Specific Builds

```bash
# Linux (AppImage and .deb)
npm run electron:build:linux

# macOS (DMG and ZIP)
npm run electron:build:mac

# Windows (NSIS installer and ZIP)
npm run electron:build:win

# All platforms (if on appropriate host)
npm run electron:build:all
```

### Build Outputs

Built packages are saved to `frontend/release/`:

- **Linux**: `LinuxCAD-<version>.AppImage` and `LinuxCAD-<version>.deb`
- **macOS**: `LinuxCAD-<version>-mac-<arch>.dmg` and `LinuxCAD-<version>-mac-<arch>.zip`
- **Windows**: `LinuxCAD-<version>-win-<arch>.exe` and `LinuxCAD-<version>-win-<arch>.zip`

## Architecture Overview

### Components

1. **Frontend** (`frontend/`)
   - Electron main process (`electron/main.cjs`)
   - React-based UI (`src/`)
   - Build configuration (`package.json`)

2. **Rust Backend** (`backend-rust/`)
   - High-performance geometry engine
   - REST API server (port 8000)
   - Built as standalone binary

### Build Process

1. **Rust Backend Build**
   - Compiles to `backend-rust/target/release/server[.exe]`
   - Automatically triggered by `prebuild` npm script
   - Cross-compilation supported for CI/CD

2. **Frontend Build**
   - TypeScript compilation
   - Vite bundling
   - Electron packaging

3. **Packaging**
   - Rust backend binary included as `extraResources`
   - Platform-specific installers/packages created
   - Icons and metadata embedded

## Troubleshooting

### Common Issues

#### Rust Backend Not Found

If you see "Rust backend executable not found" in logs:

1. Verify Rust is installed: `cargo --version`
2. Build manually: `cd backend-rust && cargo build --release`
3. Check binary exists: `backend-rust/target/release/server[.exe]`

#### Build Failures

1. **Missing dependencies**: Run `npm install` and ensure Rust is installed
2. **Permission errors**: On Unix, ensure build scripts are executable
3. **Out of disk space**: Rust builds can be large; ensure sufficient space

#### Packaging Issues

1. **Binary not included**: Check `extraResources` in package.json
2. **Wrong architecture**: Ensure building on appropriate platform for target
3. **Code signing**: macOS builds may require developer certificate

### Platform-Specific Notes

#### Linux
- AppImage packages are portable and don't require installation
- .deb packages integrate with system package managers
- Requires glibc 2.28+ for compatibility

#### macOS
- Supports both Intel and Apple Silicon (universal binaries can be configured)
- Code signing required for distribution outside App Store
- Gatekeeper may block unsigned builds

#### Windows
- NSIS installer provides standard Windows installation experience
- ZIP packages are portable
- Windows Defender may scan/quarantine builds

## CI/CD Integration

The project includes GitHub Actions workflow (`.github/workflows/desktop-build-matrix.yml`) that:

1. Sets up Node.js and Rust on all platforms
2. Installs dependencies
3. Builds Rust backend
4. Creates platform packages
5. Uploads artifacts

### Customizing CI/CD

To modify the build process:

1. Edit workflow file for different platforms/versions
2. Add code signing steps for distribution
3. Configure artifact storage and release automation

## Environment Variables

### Build Configuration

- `CI=true`: Enables CI mode with appropriate caching
- `ELECTRON_BUILDER_ALLOW_UNRESOLVED_DEPENDENCIES=true`: Allows packaging with unresolved deps

### Runtime Configuration

- `ELECTRON_DEV=1`: Development mode flag
- `ELECTRON_DISABLE_GPU=1`: Disable GPU acceleration
- `ELECTRON_OZONE_PLATFORM_HINT=wayland|x11`: Linux display server selection

## Advanced Topics

### Cross-Compilation

Rust supports cross-compilation for different targets:

```bash
# Install target
rustup target add x86_64-pc-windows-gnu

# Cross-compile
cd backend-rust
cargo build --release --target x86_64-pc-windows-gnu
```

### Custom Binary Locations

Modify `resolveRustBackendPath()` in `electron/main.cjs` to support additional binary locations.

### Performance Optimization

1. **Rust Backend**
   - Use `cargo build --release` for optimized builds
   - Consider profile-guided optimization (PGO)
   - Enable specific CPU features if targeting known hardware

2. **Frontend**
   - Configure Vite build optimizations
   - Enable/disable source maps as needed
   - Use electron-builder compression options

### Security Considerations

1. **Code Signing**
   - macOS: Requires Apple Developer certificate
   - Windows: Authenticode signing recommended
   - Linux: GPG signatures for repositories

2. **Sandboxing**
   - Electron security best practices
   - Minimize Node.js integration
   - Use context isolation

## Getting Help

For build issues:

1. Check GitHub Issues for similar problems
2. Verify all prerequisites are installed
3. Try building components individually
4. Check platform-specific requirements

For development setup:

1. Use the development build (`npm run electron:dev`)
2. Check browser dev tools for frontend issues
3. Monitor Rust backend logs for API issues
4. Use safe mode (`npm run electron:dev:safe`) for GPU issues