#!/usr/bin/env bash
# Build LinuxCAD on Linux. Wraps FreeCAD's CMake.
#
# Usage:
#   build/build-linux.sh                # configure + build
#   build/build-linux.sh --install      # configure + build + install to build/_install
#   build/build-linux.sh --reconfigure  # wipe build dir before configuring

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
# shellcheck disable=SC1091
source "$SCRIPT_DIR/common.sh"

DO_INSTALL=0
RECONFIGURE=0
for arg in "$@"; do
    case "$arg" in
        --install) DO_INSTALL=1 ;;
        --reconfigure) RECONFIGURE=1 ;;
        --help|-h)
            echo "Usage: $0 [--install] [--reconfigure]"
            exit 0
            ;;
    esac
done

if [ "$RECONFIGURE" = "1" ] && [ -d "$BUILD_DIR" ]; then
    echo "Removing existing build dir: $BUILD_DIR"
    rm -rf "$BUILD_DIR"
fi

ensure_submodules
apply_branding

echo "Configuring LinuxCAD ($LINUXCAD_VERSION) for Linux..."
cmake_configure \
    -DCMAKE_INSTALL_LIBDIR=lib \
    -DBUILD_DESIGNER_PLUGIN=OFF

echo "Building with $LINUXCAD_JOBS jobs..."
cmake_build

if [ "$DO_INSTALL" = "1" ]; then
    echo "Installing to $INSTALL_DIR..."
    cmake_install
fi

echo "Done. Build dir: $BUILD_DIR"
