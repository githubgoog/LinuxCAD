# LinuxCAD

[![Desktop Build Matrix](https://github.com/githubgoog/LinuxCAD/actions/workflows/desktop-build-matrix.yml/badge.svg)](https://github.com/githubgoog/LinuxCAD/actions/workflows/desktop-build-matrix.yml)
[![Release Publish](https://github.com/githubgoog/LinuxCAD/actions/workflows/release-publish.yml/badge.svg)](https://github.com/githubgoog/LinuxCAD/actions/workflows/release-publish.yml)

> Friendly CAD for Linux — modern shell, full FreeCAD modeling underneath.

LinuxCAD is a desktop CAD app with a refreshed top bar, an integrated
project manager, a welcome screen, and light/dark themes — built on the
proven FreeCAD modeling engine (3D viewport, sketcher, PartDesign, Part,
Draft, TechDraw, Assembly, Sheet Metal, FEM, Path).

---

## Install (recommended)

One command. No compiling, no apt packages.

```bash
curl -fsSL https://raw.githubusercontent.com/githubgoog/LinuxCAD/main/install.sh | bash
```

This downloads the latest LinuxCAD AppImage from
[GitHub Releases](https://github.com/githubgoog/LinuxCAD/releases/latest)
into `~/.local/bin/`, registers a desktop entry, and sets up an icon.

After it finishes, open your application menu and search **LinuxCAD** — or
run:

```bash
linuxcad
```

If the repository has no published GitHub Release yet, install from a direct URL
or local file instead:

```bash
bash install.sh --appimage-url "https://example.com/LinuxCAD-1.0.0-x86_64.AppImage"
# or
bash install.sh --local-appimage "/path/to/LinuxCAD-1.0.0-x86_64.AppImage"
```

To upgrade later, just re-run the same `curl ... | bash` command. To remove:

```bash
bash install.sh --uninstall
```

### Manual download

Prefer to grab a file yourself? Open
[Releases](https://github.com/githubgoog/LinuxCAD/releases/latest), download
the AppImage matching your CPU (`x86_64` or `aarch64`), then:

```bash
chmod +x LinuxCAD-*.AppImage
./LinuxCAD-*.AppImage
```

Most distros work out of the box. If launch fails with a FUSE error, install
`libfuse2` (`sudo apt install libfuse2` on Debian/Ubuntu).

### macOS / Windows

Bring-your-own builds are produced by CI — see the latest release page. A
polished installer story for macOS and Windows is on the roadmap.

---

## Repository layout

```
LinuxCAD/
├── install.sh                 # One-shot installer (downloads AppImage)
├── linuxcad                   # User launcher (AppImage-only, safe defaults)
├── linuxcad-dev               # Developer launcher (can build from source)
├── engine/                    # Vendored FreeCAD source (the modeling engine)
│   ├── LINUXCAD.md            # Why this folder exists
│   └── src/Gui/LinuxCAD/      # LinuxCAD shell module: top bar, project
│                              # manager, welcome screen, palette, theme
├── branding/                  # LinuxCAD icons, splash, themes (drop-in)
├── build/                     # CMake wrapper scripts (Linux/macOS/Windows)
├── packaging/                 # AppImage, .deb, .dmg, NSIS scripts + NOTICES
├── patches/                   # Audit trail of changes to the engine
├── scripts/                   # Developer helpers (dependency installer)
└── .github/workflows/         # CI matrix and release publishing
```

---

## Developing

Most users do **not** need this — just use `install.sh`. This section is for
contributors building LinuxCAD from source.

### Linux

```bash
bash scripts/install-linux-deps.sh         # build dependencies
./build/build-linux.sh --install            # configure, build, install to build/_install
./packaging/linux/build-appimage.sh         # produce a LinuxCAD-*.AppImage
```

Developer entry points:

- `./linuxcad` launches AppImage only (no compile fallback).
- `./linuxcad --self-check` prints diagnostics for user installs.
- `./linuxcad-dev` runs repo binaries and can build from source if missing.

For deeper notes (paths with spaces, dev menu entries, Shiboken warnings),
see [LAUNCH.md](LAUNCH.md).

### macOS

```bash
brew install cmake qt@6 boost eigen swig coin3d opencascade xerces-c fmt yaml-cpp
./build/build-mac.sh --install
./packaging/macos/build-dmg.sh
```

### Windows

```powershell
# Download a FreeCAD LibPack (1.1.x) and unpack it locally:
pwsh build/build-win.ps1 -Install -LibPack "C:\path\to\LibPack"
pwsh packaging/windows/build-nsis.ps1
```

CI runs all three flows automatically — see
[.github/workflows/desktop-build-matrix.yml](.github/workflows/desktop-build-matrix.yml).

### Branding (drop-in)

Place the assets listed in [branding/icons/README.md](branding/icons/README.md)
into `branding/icons/`. The build scripts run
[branding/apply-branding.sh](branding/apply-branding.sh) before CMake to
overlay them onto the engine's defaults — no source edits required.

### Engine modifications

LinuxCAD's UI shell lives at [engine/src/Gui/LinuxCAD/](engine/src/Gui/LinuxCAD/).
A small set of hook points in the engine call into it; the engine's own UI
(sketcher, task panels, viewport, tree, properties) is unchanged. Full audit
trail in [patches/README.md](patches/README.md) and
[engine/LINUXCAD.md](engine/LINUXCAD.md).

---

## Credits

LinuxCAD is built on the FreeCAD modeling engine (LGPL-2.1+). The vendored
engine source lives in [engine/](engine/) and retains its upstream README,
LICENSE, and history. Third-party components (OpenCASCADE, Coin3D, Qt, etc.)
are listed in [packaging/NOTICES.md](packaging/NOTICES.md).

## License

LGPL-2.1-or-later. See [LICENSE](LICENSE).
