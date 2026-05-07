#!/usr/bin/env bash
# Build LinuxCAD on macOS. Wraps FreeCAD's CMake.
#
# Usage:
#   build/build-mac.sh                # configure + build
#   build/build-mac.sh --install      # configure + build + install
#   build/build-mac.sh --reconfigure  # wipe build dir before configuring

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
    esac
done

if [ "$RECONFIGURE" = "1" ] && [ -d "$BUILD_DIR" ]; then
    rm -rf "$BUILD_DIR"
fi

ensure_submodules
apply_branding

# Default Homebrew prefix(es) get picked up via CMAKE_PREFIX_PATH.
BREW_PREFIX="${HOMEBREW_PREFIX:-$(brew --prefix 2>/dev/null || true)}"

echo "Configuring LinuxCAD ($LINUXCAD_VERSION) for macOS..."
cmake_configure \
    ${BREW_PREFIX:+"-DCMAKE_PREFIX_PATH=$BREW_PREFIX"} \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET:-12.0}" \
    -DBUILD_DESIGNER_PLUGIN=OFF

cmake_build

if [ "$DO_INSTALL" = "1" ]; then
    cmake_install
fi
