#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_MAIN="${SCRIPT_DIR}/linuxcad.svg"
SRC_GLYPH="${SCRIPT_DIR}/linuxcad-glyph.svg"

render_png() {
    local w="$1"
    local h="$2"
    local src="$3"
    local dst="$4"
    if command -v rsvg-convert >/dev/null 2>&1; then
        rsvg-convert -w "$w" -h "$h" -o "$dst" "$src"
        return 0
    fi
    if command -v magick >/dev/null 2>&1; then
        magick convert -background none -density 384 -resize "${w}x${h}" "$src" "$dst"
        return 0
    fi
    return 1
}

if ! command -v rsvg-convert >/dev/null 2>&1 && ! command -v magick >/dev/null 2>&1; then
    echo "neither rsvg-convert nor magick available; skipping PNG generation"
    exit 0
fi

for size in 16 32 48 64 128 256 512; do
    render_png "$size" "$size" "$SRC_MAIN" "${SCRIPT_DIR}/linuxcad-${size}.png"
done

render_png 16 16 "$SRC_GLYPH" "${SCRIPT_DIR}/linuxcad-glyph-16.png"
render_png 24 24 "$SRC_GLYPH" "${SCRIPT_DIR}/linuxcad-glyph-24.png"

echo "branding/icons: PNG icons generated under ${SCRIPT_DIR}"
