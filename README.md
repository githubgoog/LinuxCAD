# LinuxCAD

[![Desktop Build Matrix](https://github.com/githubgoog/LinuxCAD/actions/workflows/desktop-build-matrix.yml/badge.svg)](https://github.com/githubgoog/LinuxCAD/actions/workflows/desktop-build-matrix.yml)
[![Release Publish](https://github.com/githubgoog/LinuxCAD/actions/workflows/release-publish.yml/badge.svg)](https://github.com/githubgoog/LinuxCAD/actions/workflows/release-publish.yml)

> Friendly CAD on every desktop — modern shell, full FreeCAD modeling underneath.

LinuxCAD is a desktop CAD app with a refreshed top bar, an integrated
project manager, a welcome screen, and light/dark themes. The 3D
modeling engine (sketcher, PartDesign, Part, Draft, TechDraw, Assembly,
Sheet Metal, FEM, Path) is FreeCAD.

---

## Download

Three products, one source tree. Pick the one for your OS.

### LinuxCAD for Linux

```bash
curl -fsSL https://raw.githubusercontent.com/githubgoog/LinuxCAD/main/install.sh | bash
```

The installer purges any old install, downloads
`LinuxCAD-for-Linux-<version>-<arch>.AppImage` from the
[latest release](https://github.com/githubgoog/LinuxCAD/releases/latest)
into `~/.local/bin/`, registers a desktop entry, and adds a
`linuxcad` command to your shell. CPU is auto-detected (`x86_64` or
`aarch64`).

After install, open the app menu and search **LinuxCAD**, or run
`linuxcad`.

### LinuxCAD for macOS

```bash
curl -fsSL https://raw.githubusercontent.com/githubgoog/LinuxCAD/main/install.command | bash
```

Or download
[`install.command`](https://raw.githubusercontent.com/githubgoog/LinuxCAD/main/install.command)
and double-click it in Finder. The installer downloads
`LinuxCAD-for-macOS-<version>-<arch>.dmg` (Apple Silicon or Intel,
auto-detected), mounts it, and copies `LinuxCAD.app` into
`/Applications`.

### LinuxCAD for Windows

```powershell
iwr -useb https://raw.githubusercontent.com/githubgoog/LinuxCAD/main/install.ps1 | iex
```

The installer downloads `LinuxCAD-for-Windows-<version>-x64.zip` from
the latest release, expands it to `%LOCALAPPDATA%\Programs\LinuxCAD`,
and creates Start menu and Desktop shortcuts.

### Manual download

Prefer to grab a file yourself? Open the
[latest release](https://github.com/githubgoog/LinuxCAD/releases/latest)
and pick:

| OS                    | Asset                                                    |
| --------------------- | -------------------------------------------------------- |
| LinuxCAD for Linux    | `LinuxCAD-for-Linux-<version>-<arch>.AppImage`           |
| LinuxCAD for macOS    | `LinuxCAD-for-macOS-<version>-<arch>.dmg`                |
| LinuxCAD for Windows  | `LinuxCAD-for-Windows-<version>-x64.zip`                 |

Each release also publishes `latest.json` — a tiny manifest the
installers use to resolve the right asset for your OS / CPU.

### Install from a custom URL

If you have a pre-built artifact (or want to test a CI build before
the release is cut):

```bash
bash install.sh    --appimage-url "https://example.com/LinuxCAD-for-Linux-1.0.0-x86_64.AppImage"
bash install.command --dmg-url    "https://example.com/LinuxCAD-for-macOS-1.0.0-arm64.dmg"
powershell -File install.ps1 -ZipUrl "https://example.com/LinuxCAD-for-Windows-1.0.0-x64.zip"
```

### Uninstall

```bash
bash install.sh --uninstall          # Linux
bash install.command --uninstall     # macOS
powershell -File install.ps1 -Uninstall  # Windows
```

---

## Repository layout

```
LinuxCAD/
├── install.sh                 # LinuxCAD for Linux installer
├── install.command            # LinuxCAD for macOS installer
├── install.ps1                # LinuxCAD for Windows installer
├── linuxcad                   # User launcher (Linux + macOS)
├── linuxcad-dev               # Developer launcher (uses pixi)
├── pixi.toml                  # One reproducible build recipe for all 3 OSes
├── engine/                    # Vendored FreeCAD source (the modeling engine)
│   ├── LINUXCAD.md            # Why this folder exists
│   └── src/Gui/LinuxCAD/      # LinuxCAD shell module
├── branding/                  # LinuxCAD icons, splash, themes
├── packaging/                 # AppImage / .dmg / .zip build scripts
├── patches/                   # Audit trail of changes to the engine
└── .github/workflows/         # PR build matrix and release publishing
```

---

## Developing

Most users do **not** need this — just use one of the installers above.
This section is for contributors building LinuxCAD from source.

### One-time setup

Install [pixi](https://pixi.sh) once. It manages every dependency
(Qt6, OpenCASCADE, Coin3D, PySide6, Eigen, ...) from `conda-forge` so
the build looks identical on Linux, macOS, and Windows. No `apt`,
`brew`, or `LibPack` required.

```bash
curl -fsSL https://pixi.sh/install.sh | bash
```

### Build & launch

```bash
./linuxcad-dev                # configure + build + install + launch
./linuxcad-dev --build-only   # don't launch
./linuxcad-dev --package      # also produce dist/LinuxCAD-for-<OS>-*
```

Equivalent pixi commands (run from the repo root):

```bash
pixi run linuxcad-release     # configure + build + install
pixi run package              # produce the OS-native artifact
pixi run launch               # run the just-built binary
```

For deeper notes, see [LAUNCH.md](LAUNCH.md).

### Branding (drop-in)

Place the assets listed in [branding/icons/README.md](branding/icons/README.md)
into `branding/icons/`. The `apply-branding` pixi task overlays them
onto the engine's defaults before CMake runs — no source edits required.

### Engine modifications

LinuxCAD's UI shell lives at
[engine/src/Gui/LinuxCAD/](engine/src/Gui/LinuxCAD/). A small set of
hook points in the engine call into it; the engine's own UI (sketcher,
task panels, viewport, tree, properties) is unchanged. Full audit
trail in [patches/README.md](patches/README.md) and
[engine/LINUXCAD.md](engine/LINUXCAD.md).

---

## Credits

LinuxCAD is built on the FreeCAD modeling engine (LGPL-2.1+). The
vendored engine source lives in [engine/](engine/) and retains its
upstream README, LICENSE, and history. Third-party components
(OpenCASCADE, Coin3D, Qt, etc.) are listed in
[packaging/NOTICES.md](packaging/NOTICES.md).

## License

LGPL-2.1-or-later. See [LICENSE](LICENSE).
