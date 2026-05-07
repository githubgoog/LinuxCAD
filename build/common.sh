#!/usr/bin/env bash
# Shared build configuration for LinuxCAD (Unix-like).
# Sourced by build/build-linux.sh and build/build-mac.sh.

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SOURCE_DIR="$ROOT_DIR/FreeCAD-main"
BUILD_DIR="${LINUXCAD_BUILD_DIR:-$ROOT_DIR/build/_out}"
INSTALL_DIR="${LINUXCAD_INSTALL_DIR:-$ROOT_DIR/build/_install}"

LINUXCAD_VERSION="${LINUXCAD_VERSION:-1.0.0}"
LINUXCAD_BUILD_TYPE="${LINUXCAD_BUILD_TYPE:-Release}"
LINUXCAD_JOBS="${LINUXCAD_JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)}"

ensure_submodules() {
    if [ ! -f "$SOURCE_DIR/src/3rdParty/GSL/.git" ] && [ ! -f "$SOURCE_DIR/src/3rdParty/GSL/include/gsl/gsl" ]; then
        echo "Submodules look missing. Running git submodule update..."
        (cd "$SOURCE_DIR" && git submodule update --init --recursive --recommend-shallow)
    fi
}

apply_branding() {
    if [ -x "$ROOT_DIR/branding/apply-branding.sh" ]; then
        "$ROOT_DIR/branding/apply-branding.sh"
    fi
}

cmake_configure() {
    mkdir -p "$BUILD_DIR" "$INSTALL_DIR"
    cmake \
        -S "$SOURCE_DIR" \
        -B "$BUILD_DIR" \
        -DCMAKE_BUILD_TYPE="$LINUXCAD_BUILD_TYPE" \
        -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
        -DBUILD_QT5=OFF \
        -DBUILD_GUI=ON \
        -DBUILD_FEM_NETGEN=OFF \
        -DBUILD_VR=OFF \
        -DBUILD_TEST_RUNNER=OFF \
        -DENABLE_DEVELOPER_TESTS=OFF \
        -DPACKAGE_VERSION_SUFFIX="-linuxcad" \
        "$@"
}

cmake_build() {
    cmake --build "$BUILD_DIR" -j "$LINUXCAD_JOBS"
}

cmake_install() {
    cmake --install "$BUILD_DIR"
}
