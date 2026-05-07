# engine/

This directory is the **vendored FreeCAD source** (pinned to tag `1.1.1`).
LinuxCAD uses it as the modeling engine: 3D viewport, sketcher, PartDesign,
Part, Draft, TechDraw, Assembly, Sheet Metal, FEM, Path, etc.

It is **not** a copy of LinuxCAD. The original FreeCAD `README.md`,
`LICENSE`, and source tree are preserved here for LGPL compliance and so we
can audit/regenerate the diff against upstream cleanly.

## Where LinuxCAD's own code lives

LinuxCAD's UI shell ships as a single self-contained module:

```
engine/src/Gui/LinuxCAD/
```

It owns the top bar, project manager dock, welcome screen, command palette,
QSS theme, and the `.lcadproj` project model, and is hooked in by:

- `engine/src/Gui/CMakeLists.txt`  (`add_subdirectory(LinuxCAD)` + sources
  appended to the `FreeCADGui` shared library)
- `engine/src/Gui/MainWindow.cpp`  (`Gui::LinuxCAD::install(this)` at the
  end of the `MainWindow` constructor)
- `engine/src/Main/MainGui.cpp`    (rebrands `ExeName`, `DesktopFileName`,
  maintainer URL)

All other LinuxCAD modifications applied to upstream FreeCAD are listed in
[../patches/README.md](../patches/README.md).

## Why we vendor instead of submodule

A flat tree means:

- Source recipients can audit every line we ship without resolving git
  pointers (LGPL distribution-friendly).
- Our CI builds reproducibly without network access to upstream.
- `engine/` and `engine/src/Gui/LinuxCAD/` live in the **same** git history
  as the rest of LinuxCAD, so PRs can touch both atomically.

## Updating the engine

To bump to a newer FreeCAD tag:

```bash
git -C engine fetch --depth=1 origin tag <new-tag>
git -C engine checkout <new-tag>
# Reapply LinuxCAD patches under engine/src/Gui/{CMakeLists.txt,MainWindow.cpp},
# engine/src/Main/MainGui.cpp, engine/cMake/* — see ../patches/README.md.
git add engine && git commit
```

## License

Everything **inside this directory** is governed by `engine/LICENSE`
(LGPL-2.1-or-later) and the per-file headers, including any LinuxCAD
additions under `engine/src/Gui/LinuxCAD/` (also LGPL-2.1-or-later).

Top-level LinuxCAD files (`branding/`, `build/`, `packaging/`, `patches/`,
`scripts/`, `linuxcad`, `install.sh`) are governed by the project root
[../LICENSE](../LICENSE) and [../packaging/NOTICES.md](../packaging/NOTICES.md).
