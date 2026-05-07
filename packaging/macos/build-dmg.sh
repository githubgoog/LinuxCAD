#!/usr/bin/env bash
# Build a LinuxCAD .dmg on macOS.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
INSTALL_DIR="${LINUXCAD_INSTALL_DIR:-$ROOT/build/_install}"
OUT_DIR="${LINUXCAD_PACKAGE_DIR:-$ROOT/build/_packages}"
VERSION="${LINUXCAD_VERSION:-1.0.0}"

mkdir -p "$OUT_DIR"
APP_BUNDLE="$INSTALL_DIR/MacOS/LinuxCAD.app"
if [ ! -d "$APP_BUNDLE" ]; then
    APP_BUNDLE="$INSTALL_DIR/LinuxCAD.app"
fi

if [ ! -d "$APP_BUNDLE" ]; then
    echo "LinuxCAD.app not found under $INSTALL_DIR." >&2
    exit 1
fi

DMG="$OUT_DIR/LinuxCAD-$VERSION.dmg"
rm -f "$DMG"
hdiutil create -volname "LinuxCAD $VERSION" -srcfolder "$APP_BUNDLE" -ov -format UDZO "$DMG"
echo "Built $DMG"
