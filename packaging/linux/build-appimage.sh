#!/usr/bin/env bash
# Build a LinuxCAD AppImage from the installed tree.
#
# Prereqs: build/build-linux.sh --install has populated build/_install.
# Requires: linuxdeploy + linuxdeploy-plugin-qt + appimagetool on PATH.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
INSTALL_DIR="${LINUXCAD_INSTALL_DIR:-$ROOT/build/_install}"
OUT_DIR="${LINUXCAD_PACKAGE_DIR:-$ROOT/build/_packages}"
APPDIR="$OUT_DIR/LinuxCAD.AppDir"
VERSION="${LINUXCAD_VERSION:-1.0.0}"

if [ ! -d "$INSTALL_DIR" ]; then
    echo "Install dir not found at $INSTALL_DIR. Run build/build-linux.sh --install first." >&2
    exit 1
fi

mkdir -p "$OUT_DIR"
rm -rf "$APPDIR"
mkdir -p "$APPDIR"

# Stage installed tree into AppDir.
cp -a "$INSTALL_DIR/." "$APPDIR/"

# Desktop file.
mkdir -p "$APPDIR/usr/share/applications"
cat > "$APPDIR/usr/share/applications/org.linuxcad.LinuxCAD.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=LinuxCAD
Comment=Friendly CAD powered by FreeCAD's engine
Exec=LinuxCAD %F
Icon=org.linuxcad.LinuxCAD
Categories=Graphics;Engineering;3DGraphics;Science;
MimeType=application/x-extension-fcstd;application/x-extension-lcadproj;
StartupNotify=true
Terminal=false
EOF

# Icon.
mkdir -p "$APPDIR/usr/share/icons/hicolor/256x256/apps"
if [ -f "$ROOT/branding/icons/linuxcad-256.png" ]; then
    cp "$ROOT/branding/icons/linuxcad-256.png" "$APPDIR/usr/share/icons/hicolor/256x256/apps/org.linuxcad.LinuxCAD.png"
elif [ -f "$INSTALL_DIR/share/icons/hicolor/256x256/apps/org.freecad.FreeCAD.png" ]; then
    cp "$INSTALL_DIR/share/icons/hicolor/256x256/apps/org.freecad.FreeCAD.png" "$APPDIR/usr/share/icons/hicolor/256x256/apps/org.linuxcad.LinuxCAD.png"
fi

# Top-level icon symlinks expected by appimagetool.
ICON_PNG="$APPDIR/usr/share/icons/hicolor/256x256/apps/org.linuxcad.LinuxCAD.png"
if [ -f "$ICON_PNG" ]; then
    cp "$ICON_PNG" "$APPDIR/org.linuxcad.LinuxCAD.png"
    cp "$ICON_PNG" "$APPDIR/.DirIcon"
fi

# AppRun
cat > "$APPDIR/AppRun" <<'APPRUN'
#!/bin/bash
HERE="$(dirname "$(readlink -f "$0")")"
export PATH="$HERE/usr/bin:$PATH"
export LD_LIBRARY_PATH="$HERE/usr/lib:$HERE/usr/lib/freecad-python3:${LD_LIBRARY_PATH:-}"
export PYTHONHOME=""
exec "$HERE/usr/bin/LinuxCAD" "$@"
APPRUN
chmod +x "$APPDIR/AppRun"

# Bundle Qt dependencies if linuxdeploy is available.
if command -v linuxdeploy >/dev/null 2>&1; then
    DEPLOY_ARGS=(
        --appdir "$APPDIR"
        --desktop-file "$APPDIR/usr/share/applications/org.linuxcad.LinuxCAD.desktop"
    )
    if command -v linuxdeploy-plugin-qt >/dev/null 2>&1; then
        DEPLOY_ARGS+=(--plugin qt)
    fi
    linuxdeploy "${DEPLOY_ARGS[@]}"
fi

# Build the AppImage.
if command -v appimagetool >/dev/null 2>&1; then
    OUT="$OUT_DIR/LinuxCAD-$VERSION-x86_64.AppImage"
    ARCH=x86_64 appimagetool "$APPDIR" "$OUT"
    echo "Built $OUT"
else
    echo "appimagetool not found; AppDir prepared at $APPDIR but not packaged." >&2
fi
