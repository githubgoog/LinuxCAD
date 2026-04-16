# LinuxCAD Rust Backend Makefile

.PHONY: all build test run clean release install dev watch profile bench

# Default target
all: build

# Development build
build:
	@echo "🔧 Building LinuxCAD Rust backend..."
	cargo build

# Release build (optimized)
release:
	@echo "🚀 Building optimized release..."
	cargo build --release

# Run tests
test:
	@echo "🧪 Running tests..."
	cargo test

# Run in development mode
run:
	@echo "▶️  Starting development server..."
	cargo run

# Run in release mode
run-release:
	@echo "🚀 Starting optimized server..."
	cargo run --release

# Development with auto-reload
dev:
	@echo "🔄 Starting development server with auto-reload..."
	@if ! command -v cargo-watch >/dev/null 2>&1; then \
		echo "Installing cargo-watch..."; \
		cargo install cargo-watch; \
	fi
	cargo watch -x run

# Watch and run tests
watch-test:
	@echo "🔄 Running tests with auto-reload..."
	@if ! command -v cargo-watch >/dev/null 2>&1; then \
		echo "Installing cargo-watch..."; \
		cargo install cargo-watch; \
	fi
	cargo watch -x test

# Performance profiling
profile:
	@echo "📊 Running performance profiler..."
	@if ! command -v cargo-profiler >/dev/null 2>&1; then \
		echo "Installing cargo-profiler..."; \
		cargo install cargo-profiler; \
	fi
	cargo profiler flamegraph --bin server

# Benchmarks
bench:
	@echo "⏱️  Running benchmarks..."
	cargo bench

# Clean build artifacts
clean:
	@echo "🧹 Cleaning build artifacts..."
	cargo clean

# Install development tools
install-tools:
	@echo "🛠️  Installing development tools..."
	cargo install cargo-watch cargo-profiler cargo-expand

# Format code
fmt:
	@echo "✨ Formatting code..."
	cargo fmt

# Lint code
lint:
	@echo "🔍 Linting code..."
	cargo clippy -- -D warnings

# Security audit
audit:
	@echo "🔒 Running security audit..."
	@if ! command -v cargo-audit >/dev/null 2>&1; then \
		echo "Installing cargo-audit..."; \
		cargo install cargo-audit; \
	fi
	cargo audit

# Check dependencies for updates
outdated:
	@echo "📦 Checking for outdated dependencies..."
	@if ! command -v cargo-outdated >/dev/null 2>&1; then \
		echo "Installing cargo-outdated..."; \
		cargo install cargo-outdated; \
	fi
	cargo outdated

# Full CI pipeline
ci: lint test audit
	@echo "✅ CI pipeline completed successfully!"

# Production deployment preparation
deploy-prep: clean release test
	@echo "🚀 Production build ready!"
	@echo "Binary location: target/release/server"

# Development setup
setup: install-tools
	@echo "🎯 Development environment setup complete!"
	@echo ""
	@echo "Available commands:"
	@echo "  make dev     - Start development server with auto-reload"
	@echo "  make test    - Run tests"
	@echo "  make bench   - Run performance benchmarks"
	@echo "  make profile - Profile performance"
	@echo "  make ci      - Run full CI pipeline"

# Help
help:
	@echo "LinuxCAD Rust Backend - Make Commands"
	@echo ""
	@echo "Build Commands:"
	@echo "  build        - Development build"
	@echo "  release      - Optimized release build"
	@echo "  clean        - Clean build artifacts"
	@echo ""
	@echo "Development Commands:"
	@echo "  run          - Run development server"
	@echo "  dev          - Run with auto-reload"
	@echo "  test         - Run tests"
	@echo "  watch-test   - Run tests with auto-reload"
	@echo ""
	@echo "Quality Commands:"
	@echo "  fmt          - Format code"
	@echo "  lint         - Lint code with clippy"
	@echo "  audit        - Security audit"
	@echo "  outdated     - Check for outdated deps"
	@echo ""
	@echo "Performance Commands:"
	@echo "  bench        - Run benchmarks"
	@echo "  profile      - Performance profiling"
	@echo ""
	@echo "Deployment Commands:"
	@echo "  deploy-prep  - Prepare for production"
	@echo "  ci           - Full CI pipeline"
	@echo ""
	@echo "Setup Commands:"
	@echo "  setup        - Setup development environment"
	@echo "  install-tools- Install development tools"