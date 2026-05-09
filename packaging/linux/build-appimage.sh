#!/usr/bin/env bash
# Build "LinuxCAD for Linux" — an AppImage produced from the pixi-installed
# tree (.pixi/envs/default). The engine binary stays named FreeCAD inside; we
# add a LinuxCAD wrapper next to it and brand everything user-facing.
#
# Inputs (all optional, sensible defaults):
#   LINUXCAD_INSTALL_DIR   conda/pixi env prefix that contains bin/FreeCAD
#                          (default: .pixi/envs/default)
#   LINUXCAD_PACKAGE_DIR   where to place the AppImage (default: dist/)
#   LINUXCAD_VERSION       version string baked into the filename
#   LINUXCAD_ARCH          x86_64 (default) or aarch64
#
# Requires: linuxdeploy + linuxdeploy-plugin-qt + appimagetool on PATH. CI
# downloads them ad-hoc; locally, install via your package manager.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
INSTALL_DIR="${LINUXCAD_INSTALL_DIR:-$ROOT/.pixi/envs/default}"
OUT_DIR="${LINUXCAD_PACKAGE_DIR:-$ROOT/dist}"
VERSION="${LINUXCAD_VERSION:-1.0.0}"
ARCH="${LINUXCAD_ARCH:-$(uname -m)}"
case "$ARCH" in
    amd64) ARCH=x86_64 ;;
    arm64) ARCH=aarch64 ;;
    x86_64|aarch64) ;;
    *)
        echo "Unsupported LINUXCAD_ARCH='$ARCH'. Expected x86_64 or aarch64 (or amd64/arm64 aliases)." >&2
        exit 1
        ;;
esac

if [ ! -x "$INSTALL_DIR/bin/FreeCAD" ]; then
    echo "FreeCAD binary not found at $INSTALL_DIR/bin/FreeCAD." >&2
    echo "Run 'pixi run linuxcad-release' first to populate the install tree." >&2
    exit 1
fi

mkdir -p "$OUT_DIR"
APPDIR="$OUT_DIR/LinuxCAD.AppDir"
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr"

# Stage the pixi env's relevant subtrees into the AppDir.
# NOTE: Mod is required for built-in workbenches; without it the app starts
# with no workbenches/modules.
for sub in bin lib share include Mod Ext; do
    if [ -d "$INSTALL_DIR/$sub" ]; then
        mkdir -p "$APPDIR/usr/$sub"
        cp -a "$INSTALL_DIR/$sub/." "$APPDIR/usr/$sub/"
    fi
done

# PySide6 can include Qt3D Python extension modules that depend on libQt63D*.so.
# Our runtime does not ship Qt3D and LinuxCAD does not use those modules, so keep
# packaging deterministic by pruning them before linuxdeploy dependency scanning.
# Use a glob over python3.* so this keeps working when pixi bumps the Python minor.
shopt -s nullglob
for pyside_dir in "$APPDIR/usr/lib/python3."*/site-packages/PySide6; do
    rm -f "$pyside_dir"/Qt3D*.so
done
shopt -u nullglob

# LinuxCAD wrapper around bin/FreeCAD — keeps the engine binary name intact
# but exposes "LinuxCAD" as the user-visible entry point.
cat > "$APPDIR/usr/bin/LinuxCAD" <<'WRAPPER'
#!/usr/bin/env bash
HERE="$(cd "$(dirname "$0")" && pwd)"
export FREECAD_USER_HOME="${FREECAD_USER_HOME:-$HOME/.LinuxCAD}"
exec "$HERE/FreeCAD" "$@"
WRAPPER
chmod +x "$APPDIR/usr/bin/LinuxCAD"

mkdir -p "$APPDIR/usr/share/applications"
cat > "$APPDIR/usr/share/applications/org.linuxcad.LinuxCAD.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=LinuxCAD
GenericName=Computer-Aided Design
Comment=Friendly CAD powered by FreeCAD's engine
Exec=LinuxCAD %F
Icon=org.linuxcad.LinuxCAD
Categories=Graphics;Engineering;3DGraphics;
MimeType=application/x-extension-fcstd;
StartupNotify=true
Terminal=false
EOF
cp "$APPDIR/usr/share/applications/org.linuxcad.LinuxCAD.desktop" \
   "$APPDIR/org.linuxcad.LinuxCAD.desktop"

if command -v desktop-file-validate >/dev/null 2>&1; then
    desktop-file-validate "$APPDIR/usr/share/applications/org.linuxcad.LinuxCAD.desktop" \
        || echo "warning: desktop-file-validate reported issues (continuing)" >&2
fi

# Stage every available LinuxCAD PNG size into the AppImage's hicolor tree so
# launchers, panels, and HiDPI shells all pick up LinuxCAD branding (not the
# upstream FreeCAD icon that comes along inside share/icons/hicolor via the
# pixi env copy above).
ICON_DST_256="$APPDIR/usr/share/icons/hicolor/256x256/apps/org.linuxcad.LinuxCAD.png"
staged_any_icon=0
largest_staged=""
for size in 16 32 48 64 128 256 512; do
    src="$ROOT/branding/icons/linuxcad-${size}.png"
    if [ -f "$src" ]; then
        dst_dir="$APPDIR/usr/share/icons/hicolor/${size}x${size}/apps"
        mkdir -p "$dst_dir"
        cp "$src" "$dst_dir/org.linuxcad.LinuxCAD.png"
        staged_any_icon=1
        largest_staged="$dst_dir/org.linuxcad.LinuxCAD.png"
    fi
done

if [ "$staged_any_icon" = "0" ]; then
    echo "warning: no branding/icons/linuxcad-<size>.png found; falling back to FreeCAD icon." >&2
    echo "         Run branding/icons/build-icons.sh to render LinuxCAD PNGs from SVG." >&2
    mkdir -p "$(dirname "$ICON_DST_256")"
    for candidate in \
        "$INSTALL_DIR/share/icons/hicolor/256x256/apps/org.freecad.FreeCAD.png" \
        "$INSTALL_DIR/share/icons/hicolor/64x64/apps/org.freecad.FreeCAD.png" \
        "$INSTALL_DIR/share/icons/hicolor/48x48/apps/org.freecad.FreeCAD.png" \
        "$INSTALL_DIR/share/icons/hicolor/32x32/apps/org.freecad.FreeCAD.png" \
        "$INSTALL_DIR/share/icons/hicolor/16x16/apps/org.freecad.FreeCAD.png"; do
        if [ -f "$candidate" ]; then
            cp "$candidate" "$ICON_DST_256"
            largest_staged="$ICON_DST_256"
            break
        fi
    done
fi

# AppImage spec wants org.<id>.png and .DirIcon at the AppDir root. Prefer the
# 256 variant if we have it; otherwise use whatever largest size we did stage.
appdir_root_icon=""
if [ -f "$ICON_DST_256" ]; then
    appdir_root_icon="$ICON_DST_256"
elif [ -n "$largest_staged" ] && [ -f "$largest_staged" ]; then
    appdir_root_icon="$largest_staged"
fi
if [ -n "$appdir_root_icon" ]; then
    cp "$appdir_root_icon" "$APPDIR/org.linuxcad.LinuxCAD.png"
    cp "$appdir_root_icon" "$APPDIR/.DirIcon"
fi

cat > "$APPDIR/AppRun" <<'APPRUN'
#!/bin/bash
HERE="$(dirname "$(readlink -f "$0")")"
export PATH="$HERE/usr/bin:$PATH"
export LD_LIBRARY_PATH="$HERE/usr/lib:$HERE/usr/lib/freecad-python3:${LD_LIBRARY_PATH:-}"
export QT_PLUGIN_PATH="$HERE/usr/lib/qt6/plugins:${QT_PLUGIN_PATH:-}"
unset PYTHONHOME
exec "$HERE/usr/bin/LinuxCAD" "$@"
APPRUN
chmod +x "$APPDIR/AppRun"

# Bundle Qt + libs if linuxdeploy is on PATH.
# LinuxCAD's pixi env is already self-contained enough for AppImage use, and
# linuxdeploy occasionally fails on optional PySide modules (QtGraphs/Qt3D/etc)
# that are not required by LinuxCAD. Allow opting out (default off) so local
# packaging remains reliable.
USE_LINUXDEPLOY="${LINUXCAD_USE_LINUXDEPLOY:-0}"
if [ "$USE_LINUXDEPLOY" = "1" ] && command -v linuxdeploy >/dev/null 2>&1; then
    DEPLOY_ARGS=(
        --appdir "$APPDIR"
        --desktop-file "$APPDIR/usr/share/applications/org.linuxcad.LinuxCAD.desktop"
        --executable "$APPDIR/usr/bin/FreeCAD"
    )
    if command -v linuxdeploy-plugin-qt >/dev/null 2>&1; then
        DEPLOY_ARGS+=(--plugin qt)
    fi
    linuxdeploy "${DEPLOY_ARGS[@]}"
fi

# Build the AppImage. New name: LinuxCAD-for-Linux-<version>-<arch>.AppImage.
if command -v appimagetool >/dev/null 2>&1; then
    OUT="$OUT_DIR/LinuxCAD-for-Linux-$VERSION-$ARCH.AppImage"
    rm -f "$OUT"
    ARCH="$ARCH" appimagetool "$APPDIR" "$OUT"
    echo "Built $OUT"
else
    echo "appimagetool not found; AppDir prepared at $APPDIR but not packaged." >&2
fi
