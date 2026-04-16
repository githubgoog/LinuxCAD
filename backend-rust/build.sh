#!/bin/bash

# LinuxCAD Rust Backend Build Script

set -e

echo "🦀 Building LinuxCAD Rust Backend..."

# Check if Rust is installed
if ! command -v cargo &> /dev/null; then
    echo "❌ Rust is not installed. Please install Rust from https://rustup.rs/"
    exit 1
fi

# Move to the Rust backend directory
cd "$(dirname "$0")"

echo "📦 Installing dependencies..."
cargo check

echo "🔧 Building in release mode..."
cargo build --release

echo "🧪 Running tests..."
cargo test

echo "✅ Build complete!"
echo ""
echo "🚀 To start the server:"
echo "   cargo run --release"
echo ""
echo "🛠️  For development with auto-reload:"
echo "   cargo install cargo-watch"
echo "   cargo watch -x run"
echo ""
echo "📊 Performance profiling:"
echo "   cargo install cargo-profiler"
echo "   cargo profiler callgrind --bin server"