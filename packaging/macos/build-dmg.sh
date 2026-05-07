#!/usr/bin/env bash
# Build "LinuxCAD for macOS" — a .dmg produced from the pixi-installed tree
# (.pixi/envs/default). We assemble a LinuxCAD.app bundle whose CFBundleName
# is "LinuxCAD" but whose CFBundleExecutable is the existing FreeCAD binary,
# wrapped by a tiny shell script.
#
# Inputs (all optional):
#   LINUXCAD_INSTALL_DIR   conda/pixi env prefix (default .pixi/envs/default)
#   LINUXCAD_PACKAGE_DIR   output dir (default dist/)
#   LINUXCAD_VERSION       version baked into the filename
#   LINUXCAD_ARCH          arm64 (default on Apple Silicon) or x86_64

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
INSTALL_DIR="${LINUXCAD_INSTALL_DIR:-$ROOT/.pixi/envs/default}"
OUT_DIR="${LINUXCAD_PACKAGE_DIR:-$ROOT/dist}"
VERSION="${LINUXCAD_VERSION:-1.0.0}"
ARCH_RAW="${LINUXCAD_ARCH:-$(uname -m)}"
case "$ARCH_RAW" in
    arm64|aarch64) ARCH=arm64 ;;
    x86_64|amd64)  ARCH=x86_64 ;;
    *) ARCH="$ARCH_RAW" ;;
esac

if [ ! -x "$INSTALL_DIR/bin/FreeCAD" ]; then
    echo "FreeCAD binary not found at $INSTALL_DIR/bin/FreeCAD." >&2
    echo "Run 'pixi run linuxcad-release' first to populate the install tree." >&2
    exit 1
fi

mkdir -p "$OUT_DIR"
APP="$OUT_DIR/LinuxCAD.app"
rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources" "$APP/Contents/Frameworks"

cat > "$APP/Contents/Info.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleName</key>            <string>LinuxCAD</string>
    <key>CFBundleDisplayName</key>     <string>LinuxCAD</string>
    <key>CFBundleIdentifier</key>      <string>io.linuxcad.LinuxCAD</string>
    <key>CFBundleVersion</key>         <string>$VERSION</string>
    <key>CFBundleShortVersionString</key><string>$VERSION</string>
    <key>CFBundleExecutable</key>      <string>LinuxCAD</string>
    <key>CFBundleIconFile</key>        <string>LinuxCAD.icns</string>
    <key>CFBundlePackageType</key>     <string>APPL</string>
    <key>LSMinimumSystemVersion</key>  <string>11.0</string>
    <key>NSHighResolutionCapable</key> <true/>
    <key>NSRequiresAquaSystemAppearance</key><false/>
</dict>
</plist>
EOF

# Stage pixi env into the app's Resources, similar to standard macOS layout.
for sub in bin lib share include; do
    if [ -d "$INSTALL_DIR/$sub" ]; then
        mkdir -p "$APP/Contents/Resources/$sub"
        cp -a "$INSTALL_DIR/$sub/." "$APP/Contents/Resources/$sub/"
    fi
done

# LinuxCAD wrapper executable inside MacOS/. Defers to the engine's FreeCAD.
cat > "$APP/Contents/MacOS/LinuxCAD" <<'WRAPPER'
#!/bin/bash
DIR="$(cd "$(dirname "$0")" && pwd)"
RES="$DIR/../Resources"
export DYLD_LIBRARY_PATH="$RES/lib:${DYLD_LIBRARY_PATH:-}"
export QT_PLUGIN_PATH="$RES/lib/qt6/plugins:${QT_PLUGIN_PATH:-}"
export FREECAD_USER_HOME="${FREECAD_USER_HOME:-$HOME/Library/Application Support/LinuxCAD}"
exec "$RES/bin/FreeCAD" "$@"
WRAPPER
chmod +x "$APP/Contents/MacOS/LinuxCAD"

if [ -f "$ROOT/branding/icons/linuxcad.icns" ]; then
    cp "$ROOT/branding/icons/linuxcad.icns" "$APP/Contents/Resources/LinuxCAD.icns"
fi

# Run macdeployqt against the wrapper if available — it'll resolve Qt
# frameworks rooted at our wrapper rather than against bin/FreeCAD.
if command -v macdeployqt >/dev/null 2>&1; then
    macdeployqt "$APP" -executable="$APP/Contents/MacOS/LinuxCAD" || true
fi

DMG="$OUT_DIR/LinuxCAD-for-macOS-$VERSION-$ARCH.dmg"
rm -f "$DMG"
hdiutil create -volname "LinuxCAD $VERSION" -srcfolder "$APP" -ov -format UDZO "$DMG"
echo "Built $DMG"
