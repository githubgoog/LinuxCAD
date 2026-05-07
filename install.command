#!/usr/bin/env bash
# LinuxCAD for macOS installer.
#
# Double-click this file in Finder, or run from Terminal:
#   curl -fsSL https://raw.githubusercontent.com/githubgoog/LinuxCAD/main/install.command | bash
#
# What it does:
#   1. Downloads the matching .dmg for your CPU (arm64 / x86_64).
#   2. Mounts it and copies LinuxCAD.app into /Applications.
#   3. Removes any older LinuxCAD.app first.
#
# After install: open Finder -> Applications -> LinuxCAD.

set -euo pipefail

REPO="${LINUXCAD_REPO:-githubgoog/LinuxCAD}"
DMG_URL_OVERRIDE="${LINUXCAD_DMG_URL:-}"
LOCAL_DMG="${LINUXCAD_LOCAL_DMG:-}"
VERSION_OVERRIDE=""
DO_UNINSTALL=0

while [ $# -gt 0 ]; do
    case "$1" in
        --uninstall) DO_UNINSTALL=1 ;;
        --version=*) VERSION_OVERRIDE="${1#--version=}" ;;
        --version) shift; VERSION_OVERRIDE="${1:-}" ;;
        --dmg-url=*) DMG_URL_OVERRIDE="${1#--dmg-url=}" ;;
        --dmg-url) shift; DMG_URL_OVERRIDE="${1:-}" ;;
        --local-dmg=*) LOCAL_DMG="${1#--local-dmg=}" ;;
        --local-dmg) shift; LOCAL_DMG="${1:-}" ;;
        --help|-h) sed -n '1,15p' "$0" | sed 's/^# \?//' ; exit 0 ;;
    esac
    shift
done

log()  { printf '[linuxcad] %s\n' "$*"; }
warn() { printf '[linuxcad] %s\n' "$*" >&2; }
die()  { warn "error: $*"; exit 1; }

APP_DST="/Applications/LinuxCAD.app"

uninstall() {
    log "Removing $APP_DST..."
    if [ -d "$APP_DST" ]; then
        rm -rf "$APP_DST" 2>/dev/null || sudo rm -rf "$APP_DST"
    fi
    log "Done."
}

if [ "$DO_UNINSTALL" = 1 ]; then uninstall; exit 0; fi

case "$(uname -m)" in
    arm64)  ARCH=arm64 ;;
    x86_64) ARCH=x86_64 ;;
    *) die "unsupported CPU: $(uname -m)" ;;
esac

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"; hdiutil detach "$MOUNT" -quiet 2>/dev/null || true' EXIT
MOUNT=""

# Discover the right .dmg.
DMG_URL=""
if [ -n "$LOCAL_DMG" ]; then
    [ -f "$LOCAL_DMG" ] || die "local dmg not found: $LOCAL_DMG"
    cp -f "$LOCAL_DMG" "$TMP/linuxcad.dmg"
elif [ -n "$DMG_URL_OVERRIDE" ]; then
    DMG_URL="$DMG_URL_OVERRIDE"
else
    if [ -n "$VERSION_OVERRIDE" ]; then
        API="https://api.github.com/repos/$REPO/releases/tags/$VERSION_OVERRIDE"
    else
        API="https://api.github.com/repos/$REPO/releases/latest"
    fi
    log "Querying GitHub: $API"
    curl -fsSL -H 'Accept: application/vnd.github+json' "$API" -o "$TMP/release.json" \
        || die "could not fetch release info from $API"

    KEY="macos_${ARCH}"
    # Prefer latest.json if the release publishes one.
    LATEST_JSON_URL="$(grep -oE '"browser_download_url"[^"]*"[^"]*latest\.json"' "$TMP/release.json"         | sed -E 's/.*"(https:[^"]+)"/\1/' | head -1)"
    if [ -n "$LATEST_JSON_URL" ] && curl -fsSL "$LATEST_JSON_URL" -o "$TMP/latest.json" 2>/dev/null; then
        DMG_URL="$(grep -oE "\"$KEY\"[[:space:]]*:[[:space:]]*\"[^\"]+\"" "$TMP/latest.json"             | sed -E 's/.*"([^"]+)"$/\1/' | head -1)"
    fi
    if [ -z "$DMG_URL" ]; then
        DMG_URL="$(grep -oE '"browser_download_url"[[:space:]]*:[[:space:]]*"[^"]+"' "$TMP/release.json"             | sed -E 's/.*"(https:[^"]+)"/\1/'             | grep -E "LinuxCAD-for-macOS-.*-${ARCH}\.dmg$" | head -1)"
    fi
    [ -n "$DMG_URL" ] || die "no LinuxCAD-for-macOS-*-${ARCH}.dmg in release"
fi

if [ -z "${DMG_URL:-}" ] && [ ! -f "$TMP/linuxcad.dmg" ]; then
    die "could not resolve a .dmg to install"
fi

if [ ! -f "$TMP/linuxcad.dmg" ]; then
    log "Downloading $(basename "$DMG_URL")..."
    curl -fL --progress-bar -o "$TMP/linuxcad.dmg" "$DMG_URL"
fi

log "Mounting dmg..."
MOUNT="$(hdiutil attach -nobrowse -mountrandom "$TMP" "$TMP/linuxcad.dmg" | awk '/\/Volumes\// {for(i=3;i<=NF;i++)printf $i" "; print ""}' | sed 's/ *$//')"
[ -n "$MOUNT" ] || die "failed to mount dmg"
APP_SRC="$MOUNT/LinuxCAD.app"
[ -d "$APP_SRC" ] || die "LinuxCAD.app not found inside dmg"

if [ -d "$APP_DST" ]; then
    log "Replacing existing $APP_DST"
    rm -rf "$APP_DST" 2>/dev/null || sudo rm -rf "$APP_DST"
fi

log "Copying LinuxCAD.app to /Applications..."
cp -R "$APP_SRC" "$APP_DST" 2>/dev/null || sudo cp -R "$APP_SRC" "$APP_DST"

xattr -dr com.apple.quarantine "$APP_DST" 2>/dev/null || true

log ""
log "LinuxCAD installed at $APP_DST"
log "Open it from Finder -> Applications, or run: open -a LinuxCAD"
