#!/usr/bin/env bash
# Copy LinuxCAD branding assets on top of the matching engine assets.
#
# This is invoked by build/build-linux.sh and build/build-mac.sh before
# CMake configure. Missing files in branding/icons/ are silently skipped,
# leaving the engine's defaults in place.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ICONS_SRC="$ROOT/branding/icons"
ICONS_DST="$ROOT/engine/src/Gui/Icons"

copy_if_present() {
    local src="$1"
    local dst="$2"
    if [ -f "$src" ]; then
        cp -f "$src" "$dst"
        echo "  branding: $(basename "$src") -> $(basename "$dst")"
    fi
}

if [ ! -d "$ICONS_DST" ]; then
    echo "branding: engine/ not found at $ICONS_DST; skipping" >&2
    exit 0
fi

echo "Applying LinuxCAD branding assets..."

# Splash and about
copy_if_present "$ICONS_SRC/linuxcad-splash.png"   "$ICONS_DST/freecadsplash.png"
copy_if_present "$ICONS_SRC/linuxcad-about.png"    "$ICONS_DST/freecadabout.png"
copy_if_present "$ICONS_SRC/linuxcad-aboutdev.png" "$ICONS_DST/freecadaboutdev.png"

# Primary app icon
copy_if_present "$ICONS_SRC/linuxcad.svg"          "$ICONS_DST/freecad.svg"
copy_if_present "$ICONS_SRC/linuxcad-doc.svg"      "$ICONS_DST/freecad-doc.svg"

# Sized PNG icons
copy_if_present "$ICONS_SRC/linuxcad-16.png"   "$ICONS_DST/freecad-icon-16.png"
copy_if_present "$ICONS_SRC/linuxcad-32.png"   "$ICONS_DST/freecad-icon-32.png"
copy_if_present "$ICONS_SRC/linuxcad-48.png"   "$ICONS_DST/freecad-icon-48.png"
copy_if_present "$ICONS_SRC/linuxcad-64.png"   "$ICONS_DST/freecad-icon-64.png"
copy_if_present "$ICONS_SRC/linuxcad-128.png"  "$ICONS_DST/freecad-icon-128.png"
copy_if_present "$ICONS_SRC/linuxcad-256.png"  "$ICONS_DST/freecad-icon-256.png"

echo "Done."
