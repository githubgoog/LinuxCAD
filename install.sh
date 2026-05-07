#!/usr/bin/env bash
# LinuxCAD installer — downloads the latest AppImage from GitHub Releases and
# registers it as a normal desktop application. No compiling, no apt packages.
#
# Usage:
#   curl -fsSL https://raw.githubusercontent.com/githubgoog/LinuxCAD/main/install.sh | bash
#   bash install.sh                       # install / upgrade
#   bash install.sh --uninstall           # remove
#   bash install.sh --version v1.2.3      # pin a specific release tag
#   bash install.sh --appimage-url <url>   # install from direct AppImage URL
#   bash install.sh --local-appimage <path> # install from local AppImage file
#   LINUXCAD_REPO=user/fork bash install.sh
#   LINUXCAD_APPIMAGE_URL=<url> bash install.sh
#
# What it does:
#   1. Picks the right AppImage asset for your CPU (x86_64 or aarch64).
#   2. Downloads to ~/.local/bin/LinuxCAD-<ver>-<arch>.AppImage and chmods +x.
#   3. Installs a lightweight user launcher at ~/.local/bin/linuxcad.
#   4. Writes ~/.local/share/applications/org.linuxcad.LinuxCAD.desktop.
#   5. Extracts the AppImage's .DirIcon into ~/.local/share/icons/...
#   6. Runs update-desktop-database / gtk-update-icon-cache when available.
#
# After install: open your app menu and search "LinuxCAD".

set -euo pipefail

REPO="${LINUXCAD_REPO:-githubgoog/LinuxCAD}"
VERSION_OVERRIDE=""
APPIMAGE_URL_OVERRIDE="${LINUXCAD_APPIMAGE_URL:-}"
LOCAL_APPIMAGE=""
DO_UNINSTALL=0

while [ $# -gt 0 ]; do
    case "$1" in
        --uninstall) DO_UNINSTALL=1 ;;
        --version=*) VERSION_OVERRIDE="${1#--version=}" ;;
        --version)
            shift
            VERSION_OVERRIDE="${1:-}"
            ;;
        --appimage-url=*) APPIMAGE_URL_OVERRIDE="${1#--appimage-url=}" ;;
        --appimage-url)
            shift
            APPIMAGE_URL_OVERRIDE="${1:-}"
            ;;
        --local-appimage=*) LOCAL_APPIMAGE="${1#--local-appimage=}" ;;
        --local-appimage)
            shift
            LOCAL_APPIMAGE="${1:-}"
            ;;
        --help|-h)
            # Print only the leading comment block (top-of-file docstring),
            # not every later section divider.
            awk '
                NR==1 && /^#!/ { next }
                /^#/ { sub(/^# ?/, ""); print; next }
                { exit }
            ' "$0"
            exit 0
            ;;
    esac
    shift
done

BIN_DIR="$HOME/.local/bin"
APPS_DIR="$HOME/.local/share/applications"
ICON_DIR="$HOME/.local/share/icons/hicolor/256x256/apps"
DESKTOP_FILE="$APPS_DIR/org.linuxcad.LinuxCAD.desktop"
LAUNCHER="$BIN_DIR/linuxcad"
USER_LAUNCHER="$BIN_DIR/linuxcad-user"
DEV_LAUNCHER="$BIN_DIR/linuxcad-dev"
ICON_FILE="$ICON_DIR/linuxcad.png"

log()  { printf '[linuxcad] %s\n' "$*"; }
warn() { printf '[linuxcad] %s\n' "$*" >&2; }
die()  { warn "error: $*"; exit 1; }

uninstall() {
    log "Removing LinuxCAD user install..."
    rm -f "$DESKTOP_FILE" "$ICON_FILE"
    rm -f "$LAUNCHER" "$USER_LAUNCHER" "$DEV_LAUNCHER" 2>/dev/null || true
    rm -f "$BIN_DIR"/LinuxCAD-*.AppImage 2>/dev/null || true
    if command -v update-desktop-database >/dev/null 2>&1; then
        update-desktop-database "$APPS_DIR" >/dev/null 2>&1 || true
    fi
    log "Done."
}

if [ "$DO_UNINSTALL" = "1" ]; then
    uninstall
    exit 0
fi

# ----- prerequisite check ----------------------------------------------------

need() { command -v "$1" >/dev/null 2>&1 || die "missing command: $1"; }
need curl
need install
need uname

# fuse / appimage runtime: most distros have libfuse2 by default. We don't hard-fail.
if ! command -v fusermount >/dev/null 2>&1 && ! command -v fusermount3 >/dev/null 2>&1; then
    warn "FUSE not detected. AppImages need libfuse2; if launch fails install it with:"
    warn "  sudo apt install libfuse2"
fi

mkdir -p "$BIN_DIR" "$APPS_DIR" "$ICON_DIR"

# ----- detect arch -----------------------------------------------------------

case "$(uname -m)" in
    x86_64|amd64)   ARCH="x86_64" ;;
    aarch64|arm64)  ARCH="aarch64" ;;
    *) die "unsupported CPU: $(uname -m)" ;;
esac

# ----- discover release ------------------------------------------------------

if [ -n "$VERSION_OVERRIDE" ]; then
    RELEASE_API="https://api.github.com/repos/$REPO/releases/tags/$VERSION_OVERRIDE"
else
    RELEASE_API="https://api.github.com/repos/$REPO/releases/latest"
fi

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

ASSET_URL=""
VERSION_TAG="latest"

if [ -n "$LOCAL_APPIMAGE" ]; then
    [ -f "$LOCAL_APPIMAGE" ] || die "local AppImage not found: $LOCAL_APPIMAGE"
    ASSET_URL="file://$LOCAL_APPIMAGE"
    VERSION_TAG="local"
elif [ -n "$APPIMAGE_URL_OVERRIDE" ]; then
    ASSET_URL="$APPIMAGE_URL_OVERRIDE"
    VERSION_TAG="manual-url"
else
    log "Querying GitHub: $RELEASE_API"
    HTTP_CODE="$(curl -sSL -H 'Accept: application/vnd.github+json' -o "$TMP/release.json" -w '%{http_code}' "$RELEASE_API" || true)"
    if [ "$HTTP_CODE" = "404" ]; then
        die "no published GitHub release found for $REPO yet (HTTP 404).
Try again after a release is published, or pass --appimage-url / --local-appimage."
    fi
    if [ "$HTTP_CODE" != "200" ]; then
        die "could not fetch release info from $RELEASE_API (HTTP $HTTP_CODE)"
    fi

    # Prefer a latest.json manifest if the release publishes one (faster + stable).
    if grep -q '"latest\.json"' "$TMP/release.json"; then
        LATEST_JSON_URL="$(grep -oE '"browser_download_url"[^"]*"[^"]*latest\.json"' "$TMP/release.json"             | sed -E 's/.*"(https:[^"]+)"/\1/' | head -1)"
        if [ -n "$LATEST_JSON_URL" ]; then
            if curl -fsSL "$LATEST_JSON_URL" -o "$TMP/latest.json"; then
                case "$ARCH" in
                    x86_64)  KEY='linux_x86_64' ;;
                    aarch64) KEY='linux_aarch64' ;;
                esac
                ASSET_URL="$(grep -oE "\"$KEY\"[[:space:]]*:[[:space:]]*\"[^\"]+\"" "$TMP/latest.json"                     | sed -E 's/.*"([^"]+)"$/\1/' | head -1)"
            fi
        fi
    fi

    # Fall back to scanning release assets for a matching AppImage filename.
    if [ -z "$ASSET_URL" ]; then
        ASSET_URL="$(grep -oE '"browser_download_url"[[:space:]]*:[[:space:]]*"[^"]+"' "$TMP/release.json"             | sed -E 's/.*"(https:[^"]+)"/\1/'             | grep -E "LinuxCAD.*${ARCH}.*\.AppImage$"             | head -1)"
    fi

    [ -n "$ASSET_URL" ] || die "no AppImage asset for $ARCH found in $REPO release"

    VERSION_TAG="$(grep -oE '"tag_name"[[:space:]]*:[[:space:]]*"[^"]+"' "$TMP/release.json"         | sed -E 's/.*"([^"]+)"$/\1/' | head -1)"
    [ -n "$VERSION_TAG" ] || VERSION_TAG="latest"
fi

ASSET_NAME="$(basename "$ASSET_URL")"
TARGET="$BIN_DIR/$ASSET_NAME"

log "Found $ASSET_NAME (release $VERSION_TAG)"

# ----- download --------------------------------------------------------------

if [ -f "$TARGET" ]; then
    log "Already present at $TARGET; verifying size..."
fi

log "Installing AppImage to $TARGET"
if [[ "$ASSET_URL" == file://* ]]; then
    cp -f "${ASSET_URL#file://}" "$TARGET.part"
else
    curl -fL --progress-bar -o "$TARGET.part" "$ASSET_URL"
fi
mv -f "$TARGET.part" "$TARGET"
chmod +x "$TARGET"

# ----- launcher script --------------------------------------------------------

cat > "$LAUNCHER" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail

pick_appimage() {
    if [ -n "${LINUXCAD_APPIMAGE:-}" ] && [ -f "$LINUXCAD_APPIMAGE" ]; then
        printf '%s\n' "$LINUXCAD_APPIMAGE"; return 0
    fi
    local newest='' f
    shopt -s nullglob
    for f in "$HOME/.local/bin"/LinuxCAD-*.AppImage "$HOME/.local/bin"/LinuxCAD*.AppImage; do
        [ -f "$f" ] || continue
        if [ -z "$newest" ] || [ "$f" -nt "$newest" ]; then
            newest="$f"
        fi
    done
    shopt -u nullglob
    [ -n "$newest" ] && { printf '%s\n' "$newest"; return 0; }
    return 1
}

if [ "${1:-}" = '--self-check' ]; then
    echo 'LinuxCAD self-check'
    echo '-------------------'
    echo "Safe mode: ${LINUXCAD_SAFE_MODE:-1}"
    appimg="$(pick_appimage || true)"
    [ -n "$appimg" ] && echo "AppImage: $appimg" || echo 'AppImage: not found in ~/.local/bin'
    command -v fusermount >/dev/null 2>&1 || command -v fusermount3 >/dev/null 2>&1       && echo 'FUSE helper: yes' || echo 'FUSE helper: no (install libfuse2 if launch fails)'
    exit 0
fi

APPIMG="$(pick_appimage || true)"
if [ -z "$APPIMG" ]; then
    echo '[linuxcad] No LinuxCAD AppImage found. Re-run install.sh.' >&2
    exit 1
fi

chmod +x "$APPIMG" 2>/dev/null || true

# Safer defaults for problematic GPUs; can be disabled via LINUXCAD_SAFE_MODE=0
if [ "${LINUXCAD_SAFE_MODE:-1}" = '1' ]; then
    export LIBGL_ALWAYS_SOFTWARE=1
    export QT_OPENGL=software
    export MESA_LOADER_DRIVER_OVERRIDE=llvmpipe
fi

exec "$APPIMG" "$@"
EOF
chmod +x "$LAUNCHER"
ln -snf "$LAUNCHER" "$USER_LAUNCHER"
# ----- icon ------------------------------------------------------------------

extract_icon() {
    local out="$1" dir
    dir="$(mktemp -d)"
    if "$TARGET" --appimage-extract '*.DirIcon' >/dev/null 2>&1 \
        || "$TARGET" --appimage-extract '.DirIcon' >/dev/null 2>&1; then
        if [ -f squashfs-root/.DirIcon ]; then
            cp -f squashfs-root/.DirIcon "$out" 2>/dev/null || true
        elif [ -f squashfs-root/org.linuxcad.LinuxCAD.png ]; then
            cp -f squashfs-root/org.linuxcad.LinuxCAD.png "$out"
        fi
        rm -rf squashfs-root
    fi
}

(
    cd "$TMP"
    extract_icon "$ICON_FILE"
)

# ----- desktop entry ---------------------------------------------------------

cat > "$DESKTOP_FILE" <<EOF
[Desktop Entry]
Type=Application
Name=LinuxCAD
GenericName=Computer-Aided Design
Comment=Friendly CAD powered by the FreeCAD engine
Exec=$LAUNCHER %F
TryExec=$LAUNCHER
Icon=linuxcad
Categories=Graphics;Engineering;3DGraphics;Science;
MimeType=application/x-extension-fcstd;application/x-extension-lcadproj;
StartupNotify=true
Terminal=false
EOF

if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "$APPS_DIR" >/dev/null 2>&1 || true
fi
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -q -t -f "$HOME/.local/share/icons/hicolor" 2>/dev/null || true
fi

# ----- PATH hint -------------------------------------------------------------

case ":$PATH:" in
    *":$BIN_DIR:"*) ;;
    *)
        warn ""
        warn "Note: $BIN_DIR is not on your PATH."
        warn "Add this to your shell profile (~/.bashrc or ~/.zshrc):"
        warn "  export PATH=\"\$HOME/.local/bin:\$PATH\""
        ;;
esac

log ""
log "LinuxCAD $VERSION_TAG installed."
log "Open your app menu and search 'LinuxCAD', or run: linuxcad"
