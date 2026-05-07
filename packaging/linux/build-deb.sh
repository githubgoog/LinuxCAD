#!/usr/bin/env bash
# Build a LinuxCAD .deb from the installed tree.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
INSTALL_DIR="${LINUXCAD_INSTALL_DIR:-$ROOT/build/_install}"
OUT_DIR="${LINUXCAD_PACKAGE_DIR:-$ROOT/build/_packages}"
VERSION="${LINUXCAD_VERSION:-1.0.0}"
ARCH="${LINUXCAD_DEB_ARCH:-amd64}"

if [ ! -d "$INSTALL_DIR" ]; then
    echo "Install dir not found at $INSTALL_DIR. Run build/build-linux.sh --install first." >&2
    exit 1
fi

PKG_NAME="linuxcad"
PKG_DIR="$OUT_DIR/${PKG_NAME}_${VERSION}_${ARCH}"
rm -rf "$PKG_DIR"
mkdir -p "$PKG_DIR/DEBIAN"
mkdir -p "$PKG_DIR/usr"
cp -a "$INSTALL_DIR/." "$PKG_DIR/usr/"

cat > "$PKG_DIR/DEBIAN/control" <<EOF
Package: $PKG_NAME
Version: $VERSION
Section: graphics
Priority: optional
Architecture: $ARCH
Depends: libqt6core6, libqt6gui6, libqt6widgets6, libqt6opengl6, libqt6svg6, python3, libocct-foundation-7.7, libocct-modeling-algorithms-7.7, libocct-modeling-data-7.7
Maintainer: LinuxCAD Maintainers <maintainers@linuxcad.local>
Description: LinuxCAD - friendly CAD powered by FreeCAD
 LinuxCAD is a downstream fork of FreeCAD with a refreshed shell:
 modern top bar, integrated project manager, welcome screen, and
 light/dark themes. The full FreeCAD modeling engine and workbenches
 are preserved unchanged.
Homepage: https://github.com/githubgoog/LinuxCAD
EOF

mkdir -p "$OUT_DIR"
dpkg-deb --build --root-owner-group "$PKG_DIR" "$OUT_DIR/${PKG_NAME}_${VERSION}_${ARCH}.deb"
echo "Built $OUT_DIR/${PKG_NAME}_${VERSION}_${ARCH}.deb"
