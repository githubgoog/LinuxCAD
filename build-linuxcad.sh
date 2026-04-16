#!/usr/bin/env bash
# LinuxCAD native desktop build script

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"

if ! command -v cargo >/dev/null 2>&1; then
    echo "Error: required command 'cargo' is not available in PATH."
    exit 1
fi

if ! command -v cmake >/dev/null 2>&1; then
    echo "Error: required command 'cmake' is not available in PATH."
    exit 1
fi

echo "LinuxCAD native desktop build"
echo "============================="
echo "Root: $ROOT_DIR"

echo "Building Rust CAD engine library..."
cargo build --release --lib --manifest-path "$ROOT_DIR/backend-rust/Cargo.toml"

echo "Configuring Qt/QML desktop frontend..."
if command -v host-spawn >/dev/null 2>&1; then
    host-spawn bash -lc "/usr/bin/cmake -S \"$ROOT_DIR/frontend-qml\" -B \"$ROOT_DIR/frontend-qml/build-release\" -DCMAKE_BUILD_TYPE=Release"
else
    cmake -S "$ROOT_DIR/frontend-qml" -B "$ROOT_DIR/frontend-qml/build-release" -DCMAKE_BUILD_TYPE=Release
fi

echo "Building Qt/QML desktop frontend..."
if command -v host-spawn >/dev/null 2>&1; then
    host-spawn bash -lc "/usr/bin/cmake --build \"$ROOT_DIR/frontend-qml/build-release\""
else
    cmake --build "$ROOT_DIR/frontend-qml/build-release"
fi

echo "Build complete."
echo "Desktop binary: $ROOT_DIR/frontend-qml/build-release/linuxcad-qml"