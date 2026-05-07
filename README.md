# LinuxCAD

[![Desktop Build Matrix](https://github.com/githubgoog/LinuxCAD/actions/workflows/desktop-build-matrix.yml/badge.svg)](https://github.com/githubgoog/LinuxCAD/actions/workflows/desktop-build-matrix.yml)
[![Release Publish](https://github.com/githubgoog/LinuxCAD/actions/workflows/release-publish.yml/badge.svg)](https://github.com/githubgoog/LinuxCAD/actions/workflows/release-publish.yml)

> Friendly CAD, powered by FreeCAD's engine.

LinuxCAD is a downstream fork of [FreeCAD](https://www.freecad.org/) that
keeps **everything** good about FreeCAD's modeling — the 3D viewport, the
sketcher, PartDesign, Part, Draft, TechDraw, Assembly, Sheet Metal, FEM,
Path — and replaces only the chrome around it with a more approachable
experience:

- A **modern top bar** with a project menu, workbench switcher, save
  indicator, and a Cmd/Ctrl-K command palette.
- A **Project Manager** dock that groups multiple parts, drawings,
  assemblies, and references into a single `.lcadproj` file.
- A **Welcome screen** with recent projects, "New project" templates, and
  a one-click route to FreeCAD's classic Start workbench when you want it.
- **Light and dark themes** that style the LinuxCAD additions without
  touching FreeCAD's task panels.

The old Electron/React/Rust prototype that lived in this repo has been
removed; LinuxCAD is now a single Qt application built directly from the
forked FreeCAD source under [FreeCAD-main/](FreeCAD-main).

## Repository layout

```
LinuxCAD/
├── FreeCAD-main/              # Vendored FreeCAD source (pinned to 1.1.1)
│   └── src/Gui/LinuxCAD/      # NEW: LinuxCAD shell module (top bar, project
│                              # manager dock, welcome screen, command
│                              # palette, theme, project model)
├── branding/                  # LinuxCAD icons, splash, themes (drop-in)
├── build/                     # Cross-platform build scripts (CMake wrappers)
├── packaging/                 # AppImage, .deb, .dmg, NSIS scripts + NOTICES
├── patches/                   # LGPL audit trail of FreeCAD modifications
├── .github/workflows/         # CI matrix building all three platforms
└── LICENSE                    # LGPL-2.1-or-later
```

## Building from source

### Linux

```bash
sudo apt install build-essential cmake ninja-build qt6-base-dev qt6-tools-dev \
                 libcoin-dev libocct-modeling-data-dev libocct-modeling-algorithms-dev \
                 libxerces-c-dev libboost-all-dev libeigen3-dev libfmt-dev \
                 libyaml-cpp-dev python3-dev libpyside6-dev libshiboken6-dev \
                 swig pkg-config

./build/build-linux.sh           # configure + build
./build/build-linux.sh --install # plus install to build/_install
./packaging/linux/build-deb.sh
./packaging/linux/build-appimage.sh
```

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

## Branding (drop-in)

Place the assets listed in [branding/icons/README.md](branding/icons/README.md)
into `branding/icons/`. The build scripts run
[branding/apply-branding.sh](branding/apply-branding.sh) before CMake to
overlay them onto FreeCAD's defaults — no source edits required.

## What we changed in FreeCAD

- Added a new module: [FreeCAD-main/src/Gui/LinuxCAD/](FreeCAD-main/src/Gui/LinuxCAD/).
- Hooked it into the existing `Gui::MainWindow` constructor with one line in
  [FreeCAD-main/src/Gui/MainWindow.cpp](FreeCAD-main/src/Gui/MainWindow.cpp).
- Added the LinuxCAD shell to the `FreeCADGui` shared library via
  [FreeCAD-main/src/Gui/CMakeLists.txt](FreeCAD-main/src/Gui/CMakeLists.txt).
- Rebranded `App::Application::Config()` strings in
  [FreeCAD-main/src/Main/MainGui.cpp](FreeCAD-main/src/Main/MainGui.cpp).

The full audit trail is in [patches/README.md](patches/README.md). FreeCAD's
own UI — sketcher, task panels, viewport, tree, properties — is **not**
modified.

## License

LGPL-2.1-or-later. See [LICENSE](LICENSE) and
[packaging/NOTICES.md](packaging/NOTICES.md) for third-party components.
