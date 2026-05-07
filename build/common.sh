#!/usr/bin/env bash
# Shared build configuration for LinuxCAD (Unix-like).
# Sourced by build/build-linux.sh and build/build-mac.sh.
#
# Optional: export LINUXCAD_ROOT=/path/to/repo before sourcing (e.g. from the
# linuxcad launcher) so paths match even when this file is not under the repo.
#
# If the repo path contains whitespace, build trees default under
# $XDG_CACHE_HOME/linuxcad/build-<hash>/ (no spaces) unless LINUXCAD_BUILD_DIR /
# LINUXCAD_INSTALL_DIR are set — avoids broken Ninja/rules and matches the
# launcher when looking for binaries.

set -euo pipefail

if [ -n "${LINUXCAD_ROOT:-}" ]; then
    ROOT_DIR="$(cd "$LINUXCAD_ROOT" && pwd)"
else
    ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
fi

SOURCE_DIR="$ROOT_DIR/engine"

_default_build="$ROOT_DIR/build/_out"
_default_install="$ROOT_DIR/build/_install"
if [ -z "${LINUXCAD_BUILD_DIR:-}" ] && [ -z "${LINUXCAD_INSTALL_DIR:-}" ]; then
    if [[ "$ROOT_DIR" =~ [[:space:]] ]]; then
        _slug="$(printf '%s' "$ROOT_DIR" | sha256sum 2>/dev/null | cut -c1-16 || true)"
        if [ -z "$_slug" ]; then
            _slug="$(printf '%s' "$ROOT_DIR" | cksum | awk '{print $1}')"
        fi
        _cache_base="${XDG_CACHE_HOME:-$HOME/.cache}/linuxcad/build-$_slug"
        _default_build="$_cache_base/_out"
        _default_install="$_cache_base/_install"
    fi
fi

BUILD_DIR="${LINUXCAD_BUILD_DIR:-$_default_build}"
INSTALL_DIR="${LINUXCAD_INSTALL_DIR:-$_default_install}"

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
